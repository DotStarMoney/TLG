#include "instrument.h"

#include <algorithm>

#include "pstruct.h"

using ::util::StatusOr;

namespace audio {
namespace {

// ASCII "TLGR"
constexpr uint32_t kTLGRTag = 0x52474C54;
// ASCII "INST"
constexpr uint32_t kINSTTag = 0x54534E49;

constexpr uint8_t kLoopFlag = 1;
constexpr uint8_t kEnvelopeFlag = 2;

pstruct InstrumentHeader{
  uint32_t tlgr_tag;
  uint32_t inst_tag;
  uint8_t splits;
};

pstruct SplitHeader{
  uint64_t res_id;
  int16_t semitone_offset;
  uint8_t mode;
};

aligned_struct(4) InstLoop {
  uint32_t loop_begin;
  uint32_t loop_end;
};

pstruct InstEnvelope{
  uint16_t attack;
  uint16_t decay;
  uint8_t sustain;
  uint16_t release;
};

} // namespace

StatusOr<std::unique_ptr<const Resource>>
    Instrument::Deserialize(std::istream* stream) {

  InstrumentHeader inst_header;
  stream->read(reinterpret_cast<char*>(&inst_header), sizeof(inst_header));
  if (!(*stream)) return util::IOError("Reading INST stream failed.");

  if (inst_header.tlgr_tag != kTLGRTag) {
    return util::FormatMismatchError("INST header does not have a valid TLGR "
        "tag.");
  }

  if (inst_header.inst_tag != kINSTTag) {
    return util::FormatMismatchError("INST header does not have a valid INST "
        "tag.");
  }

  int64_t total_bytes = 0;
  std::vector<ResourceManager::MapID> res_ids;
  std::vector<Instrument::SplitData> splits;

  // Used to verify that semitone offsets are actually in increasing order.
  int16_t last_offset = 0;
  bool no_last_offset = true;
  
  if (inst_header.splits == 0) {
    return util::FormatMismatchError("INST # splits must be > 0");
  }

  for (int cur_split; cur_split < inst_header.splits; ++cur_split) {
    SplitHeader split_header;
    stream->read(reinterpret_cast<char*>(&split_header), sizeof(split_header));
    if (!(*stream)) return util::IOError("Reading INST stream failed.");
    
    if (no_last_offset) {
      no_last_offset = false;
    } else if (split_header.semitone_offset <= last_offset) {
      return util::FormatMismatchError("INST split pitch offset must be in "
          "increasing order.");
    }
    last_offset = split_header.semitone_offset;

    res_ids.emplace_back(split_header.res_id);
    
    // If there is no loop or envelope, we push back a nullptr for the split
    // characteristic and continue. 
    if (!(split_header.mode & kLoopFlag) && 
        !(split_header.mode & kEnvelopeFlag)) {
      SplitData split_data(nullptr, split_header.semitone_offset, cur_split);
      splits.push_back(split_data);
      continue;
    }

    InstrumentCharacteristics::LoopInfo loop =
        InstrumentCharacteristics::DefaultLoopInfo;
    InstrumentCharacteristics::ADSRSeconds envelope =
        InstrumentCharacteristics::DefaultEnvelope;

    if (split_header.mode & kLoopFlag) {
      loop.mode = InstrumentCharacteristics::LoopMode::LOOP;
      InstLoop inst_loop;
      stream->read(reinterpret_cast<char*>(&inst_loop), sizeof(InstLoop));
      if (!(*stream)) return util::IOError("Reading INST stream failed.");

      if (inst_loop.loop_begin >= inst_loop.loop_end) {
        return util::FormatMismatchError("INST loop start equals or exceeds "
            "loop end.");
      }
      loop.bounds.begin = inst_loop.loop_begin;
      loop.bounds.length = inst_loop.loop_end - inst_loop.loop_begin + 1;
    }

    if (split_header.mode & kEnvelopeFlag) {
      InstEnvelope inst_envelope;;
      stream->read(reinterpret_cast<char*>(&inst_envelope),
          sizeof(InstEnvelope));
      if (!(*stream)) return util::IOError("Reading INST stream failed.");

      envelope.attack = static_cast<float>(inst_envelope.attack) / 1000.0f;
      envelope.decay = static_cast<float>(inst_envelope.decay) / 1000.0f;
      envelope.sustain = static_cast<float>(inst_envelope.sustain) / 255.0f;
      envelope.release = static_cast<float>(inst_envelope.release) / 1000.0f;
    }

    SplitData split_data(new CharacteristicInfo(envelope, loop), 
        split_header.semitone_offset, cur_split);
    splits.push_back(split_data);

    // Record memory usage for CharacteristicInfo. We only do this if we
    // actually allocated space for one, thus why it is down here.
    total_bytes += sizeof(CharacteristicInfo);
  }
  total_bytes += splits.capacity() * sizeof(SplitData);
  total_bytes += res_ids.capacity() * sizeof(ResourceManager::MapID);
  total_bytes += sizeof(Instrument);

  Instrument* instrument = new Instrument(total_bytes);
  instrument->splits_ = std::move(splits);
  instrument->res_ids_ = std::move(res_ids);
 
  return std::unique_ptr<Instrument>(instrument);
}

int64_t Instrument::GetUsageBytes() const {
  return total_bytes_;
}

Instrument::~Instrument() {
  // Delete any split characteristics we had allocated, we do this over
  // unique_ptr to minimize complexity: this isn't really a case of "complex"
  // memory management... (who are you trying to convince?)
  for (const auto& split : splits_) {
    if (split.character != nullptr) delete(split.character);
  }
}

const Instrument::SplitData& Instrument::GetPlayCharacteristics(
    double semitones) const {
  auto split = std::lower_bound(splits_.begin(), splits_.end(), 
      static_cast<int16_t>(semitones), 
      [](const SplitData& a, const SplitData& b) { 
          return a.base_offset < b.base_offset; 
      });

  // If we request a split before the lowest ranged split we track, just return 
  // the split in the lowest range.
  if (split == splits_.end()) return splits_.front();

  return *split;
}

const ResourceManager::MapID& Instrument::GetSampleID(uint8_t index) const {
  return res_ids_[index];
}
} // namespace audio
