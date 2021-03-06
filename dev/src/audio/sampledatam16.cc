#include "sampledatam16.h"

#include <math.h>

#include "brr_file.h"
#include "status_macros.h"

using ::util::StatusOr;

namespace audio {

StatusOr<std::unique_ptr<const Resource>> 
    SampleDataM16::Deserialize(std::istream* stream) {
  //
  // For now, only BRR files are supported
  //
  ASSIGN_OR_RETURN(auto sample, DeserializeBRR(stream));
  if (!sample.opt_for_resynth) {
    std::vector<SampleData> pyramid = {std::move(sample.sample_data)};
    return std::unique_ptr<SampleDataM16>(
        new SampleDataM16(std::move(pyramid), sample.sampling_rate));
  }
  else {
    return std::unique_ptr<SampleDataM16>(
        new SampleDataM16(
            BuildSampleDataPyramid(std::move(sample.sample_data)),
        sample.sampling_rate));
  }
}

SampleDataM16::SampleData SampleDataM16::QuadFreqSampleData(
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

int64_t SampleDataM16::GetUsageBytes() const {
  return total_bytes_;
}

int64_t SampleDataM16::GetTotalSize() const {
  int64_t total_bytes = 0;
  for (const auto& level : data_pyramid_) {
    total_bytes += level.capacity() * sizeof(int16_t);
    total_bytes += sizeof(SampleData);
  }
  total_bytes += sizeof(SampleDataM16);
  return total_bytes;
}

} // namespace audio