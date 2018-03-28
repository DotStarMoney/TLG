#include <memory>

#include "glog/logging.h"
#include "retro/fbgfx.h"
#include "retro/fbimg.h"
#include "tlg_lib/indexstagegraph.h"

using retro::FbColor32;
using retro::FbGfx;
using retro::FbImg;

using tlg_lib::IndexStageGraph;
using tlg_lib::StageGraph;


struct NodeStuff {
  StageGraph<int, NodeStuff>::Index index;
  int x;
  NodeStuff(int x) : x(x) {}
  NodeStuff(int x, StageGraph<int, NodeStuff>::Index index)
      : index(index), x(x) {}
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  auto test = StageGraph<int, NodeStuff>::Create();
  test->AddStage("StageA", std::make_unique<int>(11),
                 std::make_unique<NodeStuff>(111110));
  test->AddStage("StageB", std::make_unique<int>(222),
                 std::make_unique<NodeStuff>(222220));
  test->AddStage("StageC", std::make_unique<int>(3333),
                 std::make_unique<NodeStuff>(333330));

  IndexStageGraph<int, NodeStuff> graph(std::move(test));

  graph.GoStage("StageC");
  graph.UpdateStageContent(std::make_unique<NodeStuff>(333339));
  graph.Retain();
  graph.UpdateStageContent(
      std::make_unique<NodeStuff>(333338, graph.retained()), false, true);

  graph.GoStage("StageB");

  graph.UpdateStageContent(std::make_unique<NodeStuff>(222229));
  graph.GoStage("StageA");
  graph.Retain();
  graph.UpdateStageContent(
      std::make_unique<NodeStuff>(111119, graph.retained()), false, true);

  graph.GoStage("StageC");
  graph.UpdateStageContent(std::make_unique<NodeStuff>(333337));

  graph.GoStage("StageA");

  NodeStuff noad(0);
  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });

  LOG(INFO) << noad.x;

  graph.GoIndex(noad.index);

  graph.GoStage("StageB");
  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });

  LOG(INFO) << noad.x;

  graph.GoStage("StageC");
  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });
  // was 39
  graph.UpdateStageContent(std::make_unique<NodeStuff>(333336, noad.index));

  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });
  graph.Retain();
  LOG(INFO) << noad.x;

  graph.GoIndex(noad.index);

  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });
  LOG(INFO) << graph.stage() << ", " << noad.x;

  graph.UpdateStageContent(
      std::make_unique<NodeStuff>(333335, graph.retained()), false, true);
  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });
  graph.GoIndex(noad.index);

  LOG(INFO) << noad.x;

  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });
  graph.GoIndex(noad.index);

  graph.GetStageContent([&noad](const NodeStuff* ns) { noad = *ns; });
  LOG(INFO) << noad.x;

  /*
  FbGfx::Screen({320, 240}, false, "FbGfx", {1080, 720});

  FbGfx::Cls(FbColor32::BLACK);

  auto testImg = FbImg::FromFile("res/wodde.png");

  FbGfx::Put(*testImg.get(), {0, 0});

  FbGfx::Rect({0, 0}, {100, 200});
  FbGfx::TextParagraph("The Supple Husk Expelled Sweet Egg Wax", {0, 0},
                       {100, 200});

  FbGfx::Flip();

  system("pause");
  */
  return 0;
}
