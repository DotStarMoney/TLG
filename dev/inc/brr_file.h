#ifndef AUDIO_BRR_FILE_H_
#define AUDIO_BRR_FILE_H_

#include <memory>
#include <string_view>
#include <vector>

#include "monosampledata16.h"
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
//     4 |         ASCII "BRR "
// ____________________________
//     2 |        sampling rate
// ____________________________
//     4 |         # of samples
// ____________________________
//     4 |  # bytes in BRR Data
// ____________________________
//     1 |   mode: bit 1 is set 
//       | if looping, bit 2 is
//       |      set if envelope
// ____________________________
//    *4 |   if loop: inclusive 
//       | sample index of loop
//       |                start
// ____________________________
//    *4 |   if loop: inclusive 
//       | sample index of loop
//       |                  end
// ____________________________
//    *2 |  if envelope: attack 
//       |                in ms
// ____________________________
//    *2 |   if envelope: decay 
//       |                in ms
// ____________________________
//    *1 | if envelope: sustain 
//       | as a % from 0 to 255             
// ____________________________
//    *2 | if envelope: release 
//       |                in ms
// ____________________________
//     4 |  # bytes in BRR Data
// ____________________________
//     * |             BRR Data
//

namespace audio {

util::StatusOr<std::unique_ptr<MonoSampleData16>> LoadBRR(
    std::string_view filename);

// If no loop, pass loop_start = loop_end = 0.
// If no envelope, pass attack = sustain = decay = release = 0
util::Status SaveBRR(std::string_view filename,
    const std::vector<int16_t>& samples, int sampling_rate, 
    uint32_t loop_start = 0, uint32_t loop_end = 0, uint16_t attack = 0,
    uint8_t decay = 0, uint16_t sustain = 0, uint16_t release = 0);

} // namespace audio

#endif // AUDIO_BRR_FILE_H_