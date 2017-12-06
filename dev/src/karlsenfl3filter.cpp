#include "karlsenfl3filter.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace audio {

KarlsenFL3Filter::KarlsenFL3Filter(SampleRate sample_rate) : Filter(),
    freq_to_normalized_mult_(sample_rate * M_1_PI * 0.5), poles_({0, 0, 0, 0}), 
    cutoff_(0), resonance_(0) {}

double KarlsenFL3Filter::ApplyLowpass(double sample) {
  sample += (sample - poles_[3]) * resonance_; 

  // Clip, adding back in some non-clipped signal for that dynamic-like analog
  // sound.
  sample += (std::clamp(sample, -1.0, 1.0) - sample) * 0.9840;

  // Straightforward 4 pole filter (4 normalized feedback paths in series)
  poles_[0] += (sample - poles_[0]) * cutoff_;
  poles_[1] += (poles_[0] - poles_[1]) * cutoff_;
  poles_[2] += (poles_[1] - poles_[2]) * cutoff_;
  poles_[3] += (poles_[2] - poles_[3]) * cutoff_;
  
  return poles_[3];
}

double KarlsenFL3Filter::Apply(double sample) {
  double filtered_sample = ApplyLowpass(sample);

  if (mode_ == Mode::kHighpass) {
    filtered_sample = sample - filtered_sample;
  }
  return filtered_sample;
}

} // namespace audio