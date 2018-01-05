#include "delayline.h"

#include <algorithm>

#include "strcat.h"

using ::util::Status;
using ::util::StrCat;

namespace audio {
namespace {
const double kMaximumDelayMS = 1000;
}

DelayLine::DelayLine(Format format, SampleSupplier<double>* input) : 
    SampleSupplier<double>(format), input_supplier_(input), 
    channels_(GetChannelLayoutChannels(format.layout)), zero_bubble_(0),
    read_from_sample_(0), old_delay_samples_(0), delay_samples_(0) {
  ASSERT_EQ(format.sample_format, SampleFormat::FLOAT64);
  ASSERT_EQ(input_supplier_->format(), format);
}

Status DelayLine::SetDelay(double delay_ms) {
  if ((delay_ms < 0.0) || (delay_ms > kMaximumDelayMS)) {
    return util::InvalidArgumentError(StrCat(
        "Delay duration out of range. 0 <= Delay <= ", kMaximumDelayMS, 
        ", but got Delay = ", delay_ms));
  }
  old_delay_samples_ = delay_samples_;
  delay_samples_ = static_cast<int32_t>((delay_ms / 1000.0) *
      format_.sampling_rate) * channels_;
  // We let the delay buffer grow dynamically as we only ever read and copy
  // from the beginning of it.
  if (delay_samples_ > delay_buffer_.size()) {
    delay_buffer_.resize(delay_samples_, 0);
  }

  const int32_t size_delta = delay_samples_ - old_delay_samples_;
  if (size_delta > 0) {
    // If the delay grows, we may end up adding more zeros if the sample we are
    // reading from in the delay buffer has not enough samples behind it to 
    // account for the new delay.
    zero_bubble_ = std::max(size_delta - read_from_sample_, 0); 
    read_from_sample_ += zero_bubble_ - size_delta;
  } else if (size_delta < 0) {
    // If the delay shrinks, we may end up shrinking our bubble of zeros if it
    // exists.
    zero_bubble_ = std::max(zero_bubble_ + size_delta, 0);
    // Futhermore, if the zero bubble didn't eat up the samples we must jump
    // ahead in time, shift the read_from_sample_ forward.
    read_from_sample_ -= size_delta - zero_bubble_;
  }
}

Status DelayLine::ProvideNextSamples(Iter samples_start, uint32_t sample_size,
    uint32_t sample_clock) {

  const uint32_t original_sample_size = sample_size;

  // Provide the bubble of zeros until its gone or we need not provide any more
  // samples.
  while ((zero_bubble_ > 0) && (sample_size > 0)) {
    for (int c = 0; c < channels_; ++c) {
      *samples_start = 0;
      ++samples_start;

      --sample_size;
      --zero_bubble_;
    }
  }
  if (sample_size == 0) return util::OkStatus;

  // Provide the delayed samples in the delay buffer (up to its size before the
  // last refresh)
  while ((read_from_sample_ < old_delay_samples_) && (sample_size > 0)) {
    for (int c = 0; c < channels_; ++c) {
      *samples_start = delay_buffer_[read_from_sample_];
      ++samples_start;

      --sample_size;
      ++read_from_sample_;
    }
  }
  if (sample_size == 0) return util::OkStatus;
  read_from_sample_ = 0;

  // While we don't care about the sample clock, our input provider might, so
  // we give them an up to date clock considering the "sample" time that has
  // passed.
  uint32_t updated_sample_clock = sample_clock + 
      (original_sample_size - sample_size) / channels_;
  // The number of samples left to provide not counting the ones we will read
  // just for our delay buffer.
  const uint32_t past_delay_samples = sample_size - delay_samples_;
  input_supplier_->ProvideNextSamples(samples_start, past_delay_samples,
      updated_sample_clock);

  updated_sample_clock += past_delay_samples / channels_;
  // Fill the delay buffer back up.
  input_supplier_->ProvideNextSamples(delay_buffer_.begin(), delay_samples_,
      updated_sample_clock);

  old_delay_samples_ = delay_samples_;
}

} // namespace audio