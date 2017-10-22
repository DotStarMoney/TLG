#ifndef AUDIO_BRR_H_
#define AUDIO_BRR_H_

// BRR (Bit Rate Reduction) compression and decompression. Provides 3.56:1
// **lossy** compression for 16b audio data.
//
// see: https://en.wikipedia.org/wiki/Bit_Rate_Reduction
//
// Uses: Mimicking SNES/PS1 and other legacy game console audio.

#include <vector>

namespace audio {

std::vector<int8_t> BRRCompress(const std::vector<int16_t>& sample_data);

// NOTE: The output will be padded with zeros so that the number of samples is 
// even.
std::vector<int16_t> BRRDecompress(const std::vector<int8_t>& comp_data);

} // namespace audio

#endif // AUDIO_BRR_H_