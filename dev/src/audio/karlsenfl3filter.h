#ifndef AUDIO_KARLSENFL3FILTER_H_
#define AUDIO_KARLSENFL3FILTER_H_

#include <array>
#include "audio_filter.h"
#include "audio_format.h"

namespace audio {

// An audio filter with resonance ( of dubious quality >:] )
//
// Code interpreted from:
// http://musicdsp.org/showArchiveComment.php?ArchiveID=240
class KarlsenFL3Filter : public Filter {
 public:
  // Sample rate is neccessary to adjust the cutoff frequency
  KarlsenFL3Filter(SampleRate sample_rate);

  // Filter cutoff frequency.
  //
  // The cutoff value is computed per the following comment from the source:
  //
  //   "Cutoff is normalized frequency in rads (2*pi*cutoff/samplerate). 
  //    Stability limit for b_cut is around 0.7-0.8."
  // 
  // We have assumed that the (capital C) Cutoff refers to the desired cutoff
  // frequency, so we have inverted the equation.
  void set_cutoff(double cutoff) { 
    cutoff_ = cutoff * freq_to_normalized_mult_; 
  }

  // Filter resonance. Set 0 to 0.4 for typical self oscillation, or 0.6+ for
  // a more saturated range. Anything from 0+ is valid.
  //
  // 0 is no resonance. Resonance is unit-less.
  void set_resonance(double resonance) { resonance_ = resonance; }

  double cutoff() const { return cutoff_; }
  double resonance() const { return resonance_; }

  double Apply(double sample) override;
 private:
  double ApplyLowpass(double sample);

  // See docs for set_cutoff
  const double freq_to_normalized_mult_;

  // Filter poles
  std::array<double, 4> poles_;

  // The computed cutoff value, note this is not a frequency.
  double cutoff_;
  double resonance_;
};

} // namespace audio

#endif // AUDIO_KARLSENFL3FILTER_H_