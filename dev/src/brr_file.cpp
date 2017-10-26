#include "brr_file.h"

#include <fstream>

#include "brr.h"
#include "pstruct.h"
#include "status_macros.h"

using ::util::StatusOr;
using ::util::Status;
using ::std::string_view;
using ::std::unique_ptr;
using ::std::vector;

namespace audio {
namespace {

// ASCII "BRR "
constexpr uint32_t kBRRTag = 0x20525242;

constexpr uint8_t kLoopFlag = 1;
constexpr uint8_t kEnvelopeFlag = 2;

pstruct BRRHeader {
  uint32_t tag;
  uint16_t sampling_rate;
  uint32_t sample_size;
  uint8_t mode;
};

aligned_struct(4) BRRLoop {
  uint32_t loop_begin;
  uint32_t loop_end;
};

pstruct BRREnvelope {
  uint16_t attack;
  uint16_t decay;
  uint8_t sustain;
  uint16_t release;
};

} // namespace

StatusOr<unique_ptr<MonoSampleData16>> LoadBRR(string_view filename,
    bool opt_for_resynth) {
  std::ifstream file(filename.data(), std::ios::binary);
  
  BRRHeader brr_header;
  file.read(reinterpret_cast<char*>(&brr_header), sizeof(BRRHeader));
  if (!file) return util::IOError("Reading BRR file failed.");

  if (brr_header.tag != kBRRTag) {
    return util::FormatMismatchError("Not a valid BRR file, tag is wrong.");
  }

  MonoSampleData16::LoopInfo loop = MonoSampleData16::DefaultLoopInfo;
  MonoSampleData16::ADSRSeconds envelope = MonoSampleData16::DefaultEnvelope;

  if (brr_header.mode & kLoopFlag) {
    loop.mode = MonoSampleData16::LoopMode::LOOP;
    BRRLoop brr_loop;
    file.read(reinterpret_cast<char*>(&brr_loop), sizeof(BRRLoop));
    if (!file) return util::IOError("Reading BRR file failed.");

    if (brr_loop.loop_begin >= brr_loop.loop_end) {
      return util::FormatMismatchError("Loop start equals or exceeds loop "
                                       "end.");
    }
    loop.bounds.begin = brr_loop.loop_begin;
    loop.bounds.length = brr_loop.loop_end - brr_loop.loop_begin + 1;
  }

  if (brr_header.mode & kEnvelopeFlag) {
    BRREnvelope brr_envelope;;
    file.read(reinterpret_cast<char*>(&brr_envelope), sizeof(BRREnvelope));
    if (!file) return util::IOError("Reading BRR file failed.");
  
    envelope.attack = static_cast<float>(brr_envelope.attack) / 1000.0f;
    envelope.decay = static_cast<float>(brr_envelope.decay) / 1000.0f;
    envelope.sustain = static_cast<float>(brr_envelope.sustain) / 255.0f;
    envelope.release = static_cast<float>(brr_envelope.release) / 1000.0f;
  }

  uint32_t data_size;
  file.read(reinterpret_cast<char*>(&data_size), sizeof(uint32_t));
  if (!file) return util::IOError("Reading BRR file failed.");

  vector<int8_t> compressed_data(data_size);
  file.read(reinterpret_cast<char*>(compressed_data.data()), data_size);
  if (!file) return util::IOError("Reading BRR file failed.");

  file.close();

  vector<int16_t> sample_data = BRRDecompress(compressed_data);
  // Since BRR blocks can be padded with an extra 0 to enforce even-ness, we
  // must check for this and resize if neccessary
  if (sample_data.size() > brr_header.sample_size) sample_data.pop_back();

  return MonoSampleData16::Create(sample_data, brr_header.sampling_rate, 
      opt_for_resynth, envelope, loop);
}

Status SaveBRR(string_view filename, const vector<int16_t>& samples,
    int sampling_rate, uint32_t loop_start, uint32_t loop_end, uint16_t attack,
    uint16_t decay, uint8_t sustain, uint16_t release) {

  const vector<int8_t> brr_data = BRRCompress(samples);
  BRRHeader brr_header{kBRRTag, static_cast<uint16_t>(sampling_rate),
      samples.size(), 0};

  if ((loop_start != 0) || (loop_end != 0)) {
    if (loop_start >= loop_end) {
      return util::FailedPreconditionError("Loop start equals or exceeds loop "
          "end.");
    }
    brr_header.mode |= kLoopFlag;
  }
  if ((attack != 0) || (decay != 0) || (sustain != 255) || (release != 0)) {
    brr_header.mode |= kEnvelopeFlag;
  }

  std::ofstream file(filename.data(), std::ios::binary);
  file.write(reinterpret_cast<const char*>(&brr_header), sizeof(brr_header));
  if (brr_header.mode & kLoopFlag) {
    const BRRLoop loop{loop_start, loop_end};
    file.write(reinterpret_cast<const char*>(&loop), sizeof(BRRLoop));
  }
  if (brr_header.mode & kEnvelopeFlag) {
    const BRREnvelope envelope{attack, decay, sustain, release};
    file.write(reinterpret_cast<const char*>(&envelope), sizeof(BRREnvelope));
  }
  const uint32_t brr_data_size = brr_data.size();
  file.write(reinterpret_cast<const char*>(&brr_data_size), sizeof(uint32_t));
  file.write(reinterpret_cast<const char*>(brr_data.data()), brr_data.size());
  file.close();

  if (!file) return util::IOError("Writing BRR file failed.");
 
  return util::OkStatus;
}
} // namespace audio