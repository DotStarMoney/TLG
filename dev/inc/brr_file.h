#ifndef AUDIO_BRR_FILE_H_
#define AUDIO_BRR_FILE_H_

#include <memory>
#include <string_view>
#include <vector>

#include "audio_format.h"
#include "sampledatam16.h"
#include "status.h"
#include "statusor.h"

// Utilities to load/save a BRR file.
//
// A BRR file is pretty simple, it's a format for storing compressed 16b mono
// audio sample data. It is expected, but not neccessary, that a BRR file ends
// with the extension ".brr". The file format is as follows:
//
// (all header numbers are considered unsigned)
//
// BYTES |       VALUE
// ============================
//     4 |         ASCII "TLGR"
// ____________________________
//     4 |         ASCII "BRR "
// ____________________________
//     2 |        sampling rate
// ____________________________
//     1 | 1 if should optimize
//       |   for resynthesis, 0
//       |            otherwise
// ____________________________
//     4 |         # of samples
// ____________________________
//     4 |  # bytes in BRR Data
// ____________________________
//     * |             BRR Data
//

namespace audio {

// The return type of DeserializeBRR
struct BRRData {
  std::vector<int16_t> sample_data;
  SampleRate sampling_rate;
  bool opt_for_resynth;
};
// Returns the BRRData deserialized into BRRData.
util::StatusOr<BRRData> DeserializeBRR(std::istream* stream);

// Saves the BRRData in the serialized brr format described above to the given
// file.
util::Status SaveBRR(std::string_view filename, const BRRData& brr_data);

} // namespace audio

#endif // AUDIO_BRR_FILE_H_