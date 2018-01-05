#ifndef AUDIO_SAMPLEDATAM16_H_
#define AUDIO_SAMPLEDATAM16_H_

#include <memory>
#include <vector>

#include "audio_format.h"
#include "assert.h"
#include "static_type_assert.h"
#include "statusor.h"
#include "resourcemanager.h"

namespace audio {

// An immutable container for sample and old-school playback data. 
//
// Another example where we use floats when doubles would be preferred only
// because low-precision might color the output sound in an infteresting way.
//
class SampleDataM16 : public Resource {
 public:
  static constexpr int64_t kResourceUID = 0x1CC8026964D338B6;
  static util::StatusOr<std::unique_ptr<const Resource>>
      Deserialize(std::istream* stream);

  int64_t resource_uid() const override { return kResourceUID; }
  int64_t GetUsageBytes() const override;

  typedef std::vector<int16_t> SampleData;

  const SampleData& data(int level) const { return data_pyramid_[level]; }
  Format format() const { return format_; }
  int pyramid_levels() const { return data_pyramid_.size(); }
 private:
  // We store sample data in a pyramid where each deeper level from 0 scales
  // the sample data down by 1/4, effectively producing a sound 2 octaves above
  // the sound produced by the sample data in the previous level. This allows
  // us to observe not more than 4 samples over a range of +8 octaves when
  // playing linearly interpolated sample data.
  static constexpr int kQuadFreqPyramidLevels = 4;

  typedef std::vector<SampleData> SampleDataPyramid;

  // We use perfect forwarding as we'd like to take advantage of users that
  // can avoid copying vector data and can move it instead.
  template <class DataPyramidT>
  SampleDataM16(DataPyramidT&& data_pyramid, int sample_rate) :
      data_pyramid_(std::forward<DataPyramidT>(data_pyramid)),
      format_({INT16, MONO, static_cast<SampleRate>(sample_rate)}),
      total_bytes_(GetTotalSize()) {
    type_assert::IsConvertibleTo<SampleDataPyramid, DataPyramidT>();
  }

  // Another example where we use perfect forwarding to avoid copying sample
  // data.

  template <class DataT>
  static SampleDataPyramid BuildSampleDataPyramid(DataT&& data) {
    SampleDataPyramid pyramid(kQuadFreqPyramidLevels);
    pyramid[0] = std::forward<DataT>(data);
    for (int level = 1; level < kQuadFreqPyramidLevels; ++level) {
      pyramid[level] = QuadFreqSampleData(pyramid[level - 1]);
    }
    return pyramid;
  }

  // Produce sample data from a source at 4x the frequency (2 octaves above).
  static SampleData QuadFreqSampleData(const SampleData& data);

  // See docs for kQuarterFrequencyPyramidLevels for information on the
  // pyramiding optimization scheme.
  const SampleDataPyramid data_pyramid_;
  const Format format_;

  // Compute the total bytes consumed by this class, requires data_pyramid_ to
  // be constructed.
  int64_t GetTotalSize() const;

  // The cached number of bytes taken up by this instrument, computed during
  // deserialization.
  const int64_t total_bytes_;
};

} // namespace audio
#endif // AUDIO_SAMPLEDATAM16_H_