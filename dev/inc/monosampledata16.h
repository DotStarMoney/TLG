#ifndef AUDIO_MONOSAMPLEDATA16_H_
#define AUDIO_MONOSAMPLEDATA16_H_

#include <array>
#include <memory>
#include <vector>

#include "audio_format.h"
#include "assert.h"
#include "static_type_assert.h"
#include "statusor.h"

// A container for sample and old-school playback data. 
//
// Another example where we use floats when doubles would be preferred only
// because low-precision might color the output sound in an interesting way.
//
// NOTE: This class does next-to-no sanity checking on incoming parameters!!
//

namespace audio {

class MonoSampleData16 {
 public:
  typedef std::vector<int16_t> SampleData;
  
  // Describes the looping playback characteristic of this sample data. 
  // ONE_SHOT describes a sample that plays straight to the end regardles of 
  // how long the sample playback is held. LOOP describes sample data that is 
  // played until the loop end point, then jumps back to the loop start if the 
  // sample data playback is still being "held" at the time it reaches the end
  // of the loop and so on...
  enum class LoopMode {
    ONE_SHOT = 0,
    LOOP = 1
  };

  // If the sample data playback should loop: these describe the point at which
  // the loop starts, and the length of the loop in samples. Obviously, these
  // values should not exceed the number of samples in the actual sample data.
  struct LoopBounds {
    double begin;
    double length;
  };
  struct LoopInfo {
    LoopMode mode;
    LoopBounds bounds;
  };

  // A time varying volume envelope describing how this sample data should be
  // played back.
  struct ADSRSeconds {
    // The time in seconds for a played sample to reach peak volume
    // starting from silence.
    float attack;
    // The time in seconds for a played sample, following the attack, to
    // reach the sustain volume.
    float decay;
    // A percentage of the sample volume at which a played sample is sustained
    // until released. 
    float sustain;
    // The time in seconds for a played sample, following playback release,
    // to reach silence from the current volume percentage.
    float release;
  };
  // Like ADSRSeconds, except time based values are in units of elapsed
  // samples.
  //
  // NOTE: These values are meaningless unless associated with a sample rate.
  typedef ADSRSeconds ADSRSamples;

  static constexpr LoopInfo DefaultLoopInfo{LoopMode::ONE_SHOT, {0.0, 0.0}};
  static constexpr ADSRSeconds DefaultEnvelope{0, 0, 1.0, 0};

  // We use perfect forwarding in the create method as this class actually
  // stores sample data and doesn't just reference it (when possible, we want
  // to MOVE vector data into it).
  template <class DataT>
  util::StatusOr<std::unique_ptr<MonoSampleData16>> Create(DataT&& data, 
      int sample_rate, bool build_pyramid = false,
      const ADSRSeconds& envelope = DefaultEnvelope, 
      const LoopInfo& loop = DefaultLoopInfo) {
    type_assert::IsConvertibleTo<ArrayType, DataT>();

    if (((loop.bounds.begin + loop.bounds.length) >=
            static_cast<ArrayType>(data).size()) || 
        (loop.bounds.begin < 0.0)) {
      return util::FailedPreconditionError("Loop bounds exceed sample data.");
    }

    SampleDataQPyramid data_pyramid;
    LoopBoundsQPyramid loop_bounds_pyramid;
    if (build_pyramid) {
      pyramid_levels_ = kQuadFreqPyramidLevels;
      data_pyramid = BuildSampleDataPyramid(std::forward<DataT>(data));
      loop_bounds_pyramid = BuildLoopBoundsPyramid(loop.bounds);
    } else {
      pyramid_levels_ = 1;
      data_pyramid = {std::forward<DataT>(data)};
      loop_bounds_pyramid = {loop.bounds};
    }

    ADSRSamples envelope = EnvSecondsToSamples(envelope, sample_rate);
 
    return std::unique_ptr<MonoSampleData16>(new MonoSampleData16(
        std::move(data_pyramid), sample_rate, envelope, loop.mode, 
        std::move(loop_bounds_pyramid)));
  }

  const SampleData& data(int level) const { return data_pyramid_[level]; }

  Format format() const { return format_; }

  const ADSRSamples& envelope() const { return envelope_; }

  bool has_loop() const { 
    return loop_mode_ == LoopMode::LOOP; 
  }
  double loop_begin(int level) const { 
    return loop_bounds_pyramid_[level].begin;
  }
  double loop_length(int level) const { 
    return loop_bounds_pyramid_[level].length;
  }

  int pyramid_levels() const { return pyramid_levels_; }

 private:
   // We store sample data in a pyramid where each deeper level from 0 scales
   // the sample data down by 1/4, effectively producing a sound 2 octaves above
   // the sound produced by the sample data in the previous level. This allows
   // us to observe not more than 4 samples over a range of +8 octaves when
   // playing linearly interpolated sample data.
   static constexpr int kQuadFreqPyramidLevels = 4;

   typedef std::array<LoopBounds, kQuadFreqPyramidLevels> LoopBoundsQPyramid;
   typedef std::array<SampleData, kQuadFreqPyramidLevels> SampleDataQPyramid;

  // More perfect forwarding...
  template <class DataPyramidT, class LoopBoundsPyramidT>
  MonoSampleData16(DataPyramidT&& data_pyramid, int sample_rate,
      const ADSRSamples& envelope, LoopMode loop_mode, 
      LoopBoundsPyramidT&& loop_bounds_pyramid) :
    data_pyramid_(std::forward<DataPyramidT>(data_pyramid)),
    loop_pyramid_(std::forward<LoopBoundsPyramidT>(loop_bounds_pyramid)),
    envelope_(envelope),
    loop_mode_(loop_mode),
    format_({INT16, MONO, sample_rate}) {
    type_assert::IsConvertibleTo<SampleDataQPyramid, DataPyramidT>();
    type_assert::IsConvertibleTo<LoopBoundsQPyramid, LoopBoundsPyramidT>();
  }

  ADSRSamples EnvSecondsToSamples(const ADSRSeconds& envelope, 
      int sampling_rate);

  // Another example where we use perfect forwarding to avoid copying sample
  // data.

  template <class DataT>
  std::array<SampleData, kQuadFreqPyramidLevels> BuildSampleDataPyramid(
      DataT&& data) {
    std::array<SampleData, kQuadFreqPyramidLevels> pyramid;
    pyramid[0] = std::forward<DataT>(data);
    for (int level = 1; level < kQuadFreqPyramidLevels; ++level) {
      pyramid[level] = QuadFreqSampleData(pyramid[level - 1]);
    }
    return pyramid;
  }

  LoopBoundsQPyramid BuildLoopBoundsPyramid(const LoopBounds& loop_bounds);

  // Produce sample data from a source at 4x the frequency (2 octaves above).
  SampleData QuadFreqSampleData(const SampleData& data);

  // See docs for kQuarterFrequencyPyramidLevels for information on the
  // pyramiding optimization scheme.
  const SampleDataQPyramid data_pyramid_;
  const LoopBoundsQPyramid loop_bounds_pyramid_;

  const ADSRSamples envelope_;
  const LoopMode loop_mode_;
  const Format format_;
  const int pyramid_levels_;
};

} // namespace audio
#endif // AUDIO_MONOSAMPLEDATA16_H_