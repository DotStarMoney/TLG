#include "brr_file.h"

#include <fstream>

#include "brr.h"
#include "pstruct.h"
#include "status_macros.h"

using ::util::StatusOr;
using ::util::Status;
using ::std::string_view;
using ::std::vector;

namespace audio {
namespace {

// ASCII "TLGR"
constexpr uint32_t kTLGRTag = 0x52474C54;
// ASCII "BRR "
constexpr uint32_t kBRRTag = 0x20525242;

pstruct BRRHeader {
  uint32_t tlgr_tag;
  uint32_t brr_tag;
  uint16_t sampling_rate;
  uint8_t mode;
  uint32_t samples_size;
  uint32_t brr_size;
};

} // namespace

StatusOr<BRRData> DeserializeBRR(std::istream* stream) {
  BRRHeader brr_header;
  stream->read(reinterpret_cast<char*>(&brr_header), sizeof(BRRHeader));
  if (!(*stream)) return util::IOError("Reading BRR file failed.");

  if (brr_header.tlgr_tag != kTLGRTag) {
    return util::FormatMismatchError("BRR header does not have a valid TLGR "
        "tag.");
  }
  if (brr_header.brr_tag != kBRRTag) {
    return util::FormatMismatchError("BRR header does not have a valid BRR "
        "tag.");
  }

  vector<int8_t> compressed_data(brr_header.brr_size);
  stream->read(reinterpret_cast<char*>(compressed_data.data()),
      brr_header.brr_size);
  if (!(*stream)) return util::IOError("Reading BRR file failed.");

  vector<int16_t> sample_data = BRRDecompress(compressed_data);
  // Since BRR blocks can be padded with an extra 0 to enforce even-ness, we
  // must check for this and resize if neccessary
  if (sample_data.size() > brr_header.samples_size) sample_data.pop_back();

  return BRRData{std::move(sample_data),
      static_cast<SampleRate>(brr_header.sampling_rate), brr_header.mode == 1};
}

Status SaveBRR(string_view filename, const BRRData& brr_data) {
  const vector<int8_t> brr_compressed_data = BRRCompress(brr_data.sample_data);
  BRRHeader brr_header{kTLGRTag, kBRRTag, 
      static_cast<uint16_t>(brr_data.sampling_rate),
      static_cast<uint8_t>(brr_data.opt_for_resynth ? 1 : 0),
      brr_data.sample_data.size(), brr_compressed_data.size()};

  std::ofstream file(filename.data(), std::ios::binary);
  file.write(reinterpret_cast<const char*>(&brr_header), sizeof(brr_header));
  file.write(reinterpret_cast<const char*>(brr_compressed_data.data()), 
      brr_compressed_data.size());
  
  file.close();
  if (!file) return util::IOError("Writing BRR file failed.");
 
  return util::OkStatus;
}
} // namespace audio