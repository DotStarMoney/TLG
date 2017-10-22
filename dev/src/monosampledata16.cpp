#include "monosampledata16.h"

#include <math.h>

namespace audio {

MonoSampleData16::ADSRSamples MonoSampleData16::EnvSecondsToSamples(
    const ADSRSeconds& envelope, int sampling_rate) {
  return {
      envelope.attack * sampling_rate,
      envelope.decay * sampling_rate,
      envelope.sustain,
      envelope.release * sampling_rate };
}

MonoSampleData16::LoopBoundsQPyramid MonoSampleData16::BuildLoopBoundsPyramid(
    const LoopBounds& loop_bounds) {
  LoopBoundsQPyramid pyramid = {loop_bounds};
  for (int level = 1; level < kQuadFreqPyramidLevels; ++level) {
    pyramid[level].begin = pyramid[level - 1].begin * 0.25;
    pyramid[level].length = pyramid[level - 1].length * 0.25;
  }
  return pyramid;
}

MonoSampleData16::SampleData MonoSampleData16::QuadFreqSampleData(
    const SampleData& data) {
  SampleData new_data(static_cast<uint32_t>(std::ceil(data.size() * 0.25)));

  // Average together blocks of 4 samples in the sample data to create the new
  // data
  uint32_t start_block = 3;
  uint32_t new_index = 0;
  for (start_block = 3, new_index = 0; 
      start_block < data.size();
      start_block += 4, ++new_index) {
    // We round instead of truncating towards zero so that we lessen the bias
    // introduced by successive scaling of sample data. Rounding should
    // introduce a slight bias away from 0 (as we divide by 4 and *.5 and *.75
    // will round up while *.25 will round down) 
    new_data[new_index] = static_cast<int16_t>(std::round(static_cast<double>(
        static_cast<int32_t>(data[start_block - 3]) + data[start_block - 2] + 
        data[start_block - 1] + data[start_block]) * 0.25));
  }

  // Return to the start of a potential last block of sample data with < 4
  // samples in it. 
  start_block -= 3;
  if (start_block >= data.size()) return new_data;

  // Account for samples we missed because the data size was not divisible by 4
  int32_t acc = 0;
  while (start_block < data.size()) {
    acc += data[start_block++];
  }
  new_data[new_index] = static_cast<int16_t>(std::round(static_cast<double>(
      acc) * 0.25));
  return new_data;
}

} // namespace audio