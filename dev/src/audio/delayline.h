#ifndef AUDIO_DELAYLINE_H_
#define AUDIO_DELAYLINE_H_

#include <vector>

#include "samplesupplier.h"
#include "status.h"

namespace audio {

// A delay line with feedback and an optional filter inline with the feedback
// path. A delay provides a classic "echo" effect.
//
// Not thread safe.
class DelayLine : public SampleSupplier<double> {
 public:
  DelayLine(Format format, SampleSupplier<double>* input);

  // Returns the delay in ms.
  double delay() const { 
    return 1000.0 * static_cast<double>(delay_samples_) / 
        format_.sampling_rate; 
  }

  // Sets the delay in ms
  util::Status SetDelay(double delay_ms);

  util::Status ProvideNextSamples(Iter samples_start, uint32_t sample_size,
      uint32_t sample_clock);
 private:
  // Not owned.
  SampleSupplier<double>* const input_supplier_;
  const int channels_;

  // The number of samples by which audio provided from this DelayLine is
  // delayed
  int32_t delay_samples_;

};

} // namespace audio

#endif // AUDIO_DELAYLINE_H_