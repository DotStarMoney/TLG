#ifndef AUDIO_AUDIOSYSTEM_H_
#define AUDIO_AUDIOSYSTEM_H_

#include <stdint.h>

#include "audio_format.h"

namespace audio {

// A top level object for managing an audio subsystem. This object must outlive
// AudioComponents that reference it.
//
// Floats are used for deliberate precision loss.
class AudioSystem {
 public:
  AudioSystem(SampleRate sample_rate);

  void set_oscillator_rate(float rate);
  // Get a value from a system wide oscillator with oscillations between -1 and
  // 1. Values retrieved from this method should approximate a sinusoid.
  float GetOscillatorValue(uint32_t elapsed_samples);
  
  SampleRate sample_rate() const { return sample_rate_; }
 private:
  const SampleRate sample_rate_;
  // An oscillator rate in Cycles / Sample.
  float oscillator_rate_;
};

// Child objects of an AudioSystem.
class AudioComponent {
 public:
  explicit AudioComponent(AudioSystem* const parent) : parent_(parent) {}
 protected:
  // Not owned.
  AudioSystem* const parent_;
};

} // namespace audio

#endif // AUDIO_AUDIOSYSTEM_H_