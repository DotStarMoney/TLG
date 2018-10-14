#include "experimental/cask.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "thread/workqueue.h"
#include "util/random.h"

DEFINE_uint32(random_place_count, 200,
              "Number of attempts made to place objects when creating "
              "candidates randomly");
DEFINE_uint32(candidates, 10000, "Number of candidates per village.");
DEFINE_uint32(villages, 20, "Number of villages.");
DEFINE_uint32(epochs, 2000000, "Number of epochs.");
DEFINE_uint32(consensus_period, 650,
              "Number of epochs between village consensus and massacre.");
DEFINE_double(massacre_percent, 0.8,
              "Percent of villages to massacre after reaching consensus.");
DEFINE_double(derived_p, 0.95,
              "When deriving a candidate, probability that a given square will "
              "be taken from the source candidate.");
DEFINE_double(percent_derived_top1, 0.4,
              "Percent of remaining candidates excluding top 2 that are "
              "derived from the top 1 candidate.");
DEFINE_double(percent_derived_top2, 0.29,
              "Percent of remaining candidates excluding top 2 that are "
              "derived from the top 2 candidate.");
DEFINE_double(percent_derived_both, 0.3,
              "Percent of remaining candidates excluding top 2 that are "
              "derived from both top 1 and top 2 candidates.");
DEFINE_double(consensus_percent, 0.95,
              "Percent consensus at which evolution ceases.");

constexpr int kEntranceSquare = 2;
constexpr int kSolidSquare = 1;
constexpr int kEmptySquare = 0;

namespace experimental {
namespace cask {
namespace {

struct CandidateList {
  CandidateList(const std::vector<int>& layout, int w, int h, int n_candidates)
      : w(w),
        h(h),
        offsets(
            {-w, 1, w, -1}),  //{-w, -w + 1, 1, w + 1, w, w - 1, -1, -w - 1}),
        layout(layout),
        visited_map(w * h, 0),
        visited_id(1),
        scores(n_candidates, 0) {
    CHECK_GT(n_candidates, 3);
    CHECK_GT(w, 4);
    CHECK_GT(h, 4);
    CHECK_EQ(layout.size(), w * h);
    for (int i = 0; i < n_candidates; ++i) {
      candidates.emplace_back(w * h, 0);
    }
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        if (layout[y * w + x] == kEntranceSquare) {
          start_x = x;
          start_y = y;
          return;
        }
      }
    }
    CHECK(false);
  }

  void SetRandomCandidate(int cand, int active_id,
                          const std::vector<int>& consensus) {
    for (int i = 0; i < FLAGS_random_place_count; ++i) {
      const int x = util::rnd() % w;
      const int y = util::rnd() % h;
      const int off = y * w + x;
      if (layout[off] == kEmptySquare) {
        candidates[cand][off] = active_id;
      }
    }
    for (int i = 0; i < w * h; ++i) {
      if (consensus[i] != 0) {
        candidates[cand][i] = (consensus[i] == 1) ? active_id : 0;
      }
    }
  }

  void SetDerivedCandidate(int src, int dst, int last_active_id, int active_id,
                           const std::vector<int>& consensus) {
    for (int i = 0; i < w * h; ++i) {
      if (layout[i] == kEmptySquare) {
        if (consensus[i] != 0) {
          if (consensus[i] == 1) {
            candidates[dst][i] = active_id;
          }
          continue;
        }
        if (util::TrueWithChance(FLAGS_derived_p)) {
          if (candidates[src][i] == last_active_id) {
            candidates[dst][i] = active_id;
          }
        } else if (util::TrueWithChance(0.5)) {
          candidates[dst][i] = active_id;
        }
      }
    }
  }

  void SetBiDerivedCandidate(int src_a, int src_b, int dst, int last_active_id,
                             int active_id, const std::vector<int>& consensus) {
    for (int i = 0; i < w * h; ++i) {
      if (layout[i] == kEmptySquare) {
        if (consensus[i] != 0) {
          if (consensus[i] == 1) {
            candidates[dst][i] = active_id;
          }
          continue;
        }
        if (util::TrueWithChance(FLAGS_derived_p)) {
          if (candidates[util::TrueWithChance(0.5) ? src_a : src_b][i] ==
              last_active_id) {
            candidates[dst][i] = active_id;
          }
        } else if (util::TrueWithChance(0.5)) {
          candidates[dst][i] = active_id;
        }
      }
    }
  }

  int Score(int cand, int active_id) {
    int score = 0;
    next_stack.push_back(start_y * w + start_x);
    visited_map[next_stack.back()] = visited_id;
    while (!next_stack.empty()) {
      const int off = next_stack.back();
      next_stack.pop_back();

      for (int i = 0; i < offsets.size(); ++i) {
        const int this_off = off + offsets[i];
        if (visited_map[this_off] != visited_id) {
          if (layout[this_off] != kSolidSquare) {
            if (candidates[cand][this_off] == active_id) {
              ++score;
              visited_map[this_off] = visited_id;
            } else {  // if ((i % 2) == 0) {
              next_stack.push_back(this_off);
              visited_map[this_off] = visited_id;
            }
          }
        }
      }
    }
    int total_obj = 0;
    for (int i = 0; i < w * h; ++i) {
      total_obj += (candidates[cand][i] == active_id) ? 1 : 0;
    }
    score -= (total_obj - score);
    return score;
  }

  void Refresh(int cand, int active_id, int last_active_id) {
    for (int i = 0; i < w * h; ++i) {
      if (candidates[cand][i] == last_active_id) {
        candidates[cand][i] = active_id;
      }
    }
  }

  void Sort(int active_id) {
    int largest = 0;
    int largest_i = 0;
    int second_largest_i = 1;
    for (int i = 0; i < candidates.size(); ++i) {
      scores[i] = Score(i, active_id);
      // Prefer a new candidate if tied for first
      if (scores[i] >= largest) {
        second_largest_i = largest_i;
        largest_i = i;
        largest = scores[i];
      }
      ++visited_id;
    }

    if (second_largest_i != largest_i) {
      candidates[1].swap(candidates[second_largest_i]);
      std::swap(scores[1], scores[second_largest_i]);
    }

    candidates[0].swap(candidates[largest_i]);
    std::swap(scores[0], scores[largest_i]);
  }

  const int w;
  const int h;
  int start_x;
  int start_y;
  const std::vector<int> offsets;
  const std::vector<int> layout;
  std::vector<int> visited_map;
  std::vector<int> next_stack;
  int visited_id;
  std::vector<std::vector<int>> candidates;
  std::vector<int> scores;
};
}  // namespace

/*
constexpr int kWidth = 20;
constexpr int kHeight = 14;

const std::vector<int> layout = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
*/
constexpr int kWidth = 25;
constexpr int kHeight = 14;

const std::vector<int> layout = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

constexpr int kArea = kWidth * kHeight;

int run() {
  std::vector<CandidateList> villages(
      FLAGS_villages, CandidateList(layout, kWidth, kHeight, FLAGS_candidates));
  std::vector<thread::WorkQueue> queues(FLAGS_villages);
  std::atomic<int> gateway(0);
  std::vector<int> consensus(kArea, 0);
  int active_id = 1;
  for (int v = 0; v < FLAGS_villages; ++v) {
    for (int i = 0; i < FLAGS_candidates; ++i) {
      villages[v].SetRandomCandidate(i, active_id, consensus);
    }
  }

  auto run_one = [&](int village) -> void {
    villages[village].Sort(active_id);
    const int last_active_id = active_id;
    const int next_active_id = active_id + 1;
    const int n_recreate = FLAGS_candidates - 2;

    int start = 2;
    int end = start + n_recreate * FLAGS_percent_derived_top1;
    for (; start < end; ++start) {
      villages[village].SetDerivedCandidate(0, start, last_active_id,
                                            next_active_id, consensus);
    }

    start = end;
    end = start + n_recreate * FLAGS_percent_derived_top2;
    for (; start < end; ++start) {
      villages[village].SetDerivedCandidate(1, start, last_active_id,
                                            next_active_id, consensus);
    }

    start = end;
    end = start + n_recreate * FLAGS_percent_derived_both;
    for (; start < end; ++start) {
      villages[village].SetBiDerivedCandidate(0, 1, start, last_active_id,
                                              next_active_id, consensus);
    }

    start = end;
    for (; start < FLAGS_candidates; ++start) {
      villages[village].SetRandomCandidate(start, next_active_id, consensus);
    }

    villages[village].Refresh(0, next_active_id, last_active_id);
    villages[village].Refresh(1, next_active_id, last_active_id);

    gateway.fetch_add(1, std::memory_order_release);
  };

  for (int e = 0; e < FLAGS_epochs; ++e) {
    for (int v = 0; v < FLAGS_villages; ++v) {
      queues[v].AddWork([&, v]() { run_one(v); });
    }
    while (gateway.load(std::memory_order_relaxed) < FLAGS_villages) {
    }
    gateway = 0;
    ++active_id;
    int best_village = 0;
    for (int v = 0; v < FLAGS_villages; ++v) {
      if (villages[v].scores[0] > villages[best_village].scores[0]) {
        best_village = v;
      }
    }
    if (e % 100 == 0) {
      std::cout << "____________________________________" << std::endl;
      std::cout << "EPOCH: " << e << std::endl;
      std::cout << "____________________________________" << std::endl;
      for (int v = 0; v < FLAGS_villages; ++v) {
        std::cout << "Village: " << (v + 1) << std::endl;
        std::cout << "------------------" << std::endl;
        std::cout << "Score: " << villages[v].scores[0] << std::endl;
        for (int y = 0; y < kHeight; ++y) {
          for (int x = 0; x < kWidth; ++x) {
            std::cout << ((villages[v].candidates[0][y * kWidth + x] ==
                           active_id)
                              ? "#"
                              : "_");
          }
          std::cout << std::endl;
        }
      }
      std::cout << "----------------------" << std::endl;
      std::cout << "High score: " << villages[best_village].scores[0]
                << std::endl;
      std::cout << "----------------------" << std::endl;
    }

    if ((e != 0) && ((e % FLAGS_consensus_period) == 0)) {
      int unresolved = 0;
      std::cout << "Updating consensus and massacring: " << std::endl;
      for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
          const int off = y * kWidth + x;
          if (consensus[off] == 0) {
            int v = 0;
            for (; v < (FLAGS_villages - 1); ++v) {
              const bool vi_on = villages[v].candidates[0][off] == active_id;
              const bool vi_1_on =
                  villages[v + 1].candidates[0][off] == active_id;
              if (vi_on != vi_1_on) {
                break;
              }
            }
            if (v == (FLAGS_villages - 1)) {
              consensus[off] =
                  (villages[0].candidates[0][off] == active_id) ? 1 : -1;
            } else {
              ++unresolved;
            }
          }
        }
      }

      const double consensus_p = static_cast<float>(kArea - unresolved) / kArea;
      if (consensus_p >= FLAGS_consensus_percent) {
        std::cout << "__________________" << std::endl;
        std::cout << "Consensus reached!" << std::endl;
        std::cout << "__________________" << std::endl;
        std::cout << std::endl;
        std::cout << "Final epoch: " << e << std::endl;
        std::cout << "Final result: " << std::endl;
        for (int y = 0; y < kHeight; ++y) {
          for (int x = 0; x < kWidth; ++x) {
            std::cout << ((villages[best_village]
                               .candidates[0][y * kWidth + x] == active_id)
                              ? "#"
                              : "_");
          }
          std::cout << std::endl;
        }
        break;
      } else {
        std::cout << "Consensus progress: " << (consensus_p * 100.0) << "%"
                  << std::endl;
      }

      for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
          const int con_v = consensus[y * kWidth + x];
          std::cout << ((con_v == 0) ? "-" : ((con_v == 1) ? "#" : "_"));
        }
        std::cout << std::endl;
      }

      std::vector<std::pair<int, int>> villages_i_s;
      for (int v = 0; v < FLAGS_villages; ++v) {
        villages_i_s.emplace_back(v, villages[v].scores[0]);
      }
      std::sort(villages_i_s.begin(), villages_i_s.end(),
                [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                  return a.second < b.second;
                });

      int v = 0;
      for (; v < static_cast<int>(FLAGS_villages * FLAGS_massacre_percent);
           ++v) {
        for (int i = 0; i < FLAGS_candidates; ++i) {
          const int index = villages_i_s[v].first;
          std::fill(villages[index].candidates[i].begin(),
                    villages[index].candidates[i].end(), 0);
          villages[index].SetRandomCandidate(i, active_id, consensus);
        }
      }
      std::cout << "Massacred " << (v - 1) << " villages." << std::endl;
    }
  }

  std::cin.ignore();
  return 0;
}
}  // namespace cask
}  // namespace experimental
