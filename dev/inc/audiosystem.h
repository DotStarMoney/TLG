#ifndef AUDIO_AUDIOSYSTEM_H_
#define AUDIO_AUDIOSYSTEM_H_

#include <stdint.h>
#include <string_view>

#include "audio_format.h"
#include "status.h"

// Individual sampler commands and global state commands get buffered, then 
// all that were buffered are cleared from the buffer.

// Master loop looks like:
//
//  Game logic (threaded) {
//    do audio stuff
//  }
//
//  AudioSystem.FeedAudio(1/60th of second) {
//    freeze # of buffered commands to absorb.
//    apply commands
//    traverse audio pipeline (as threaded as desired)
//  }
//  
//  
// AudioSystem has a thread pool, and also reduces/increases the number of
// samples (by 1) to pull to try and keep the # of already buffered samples as
// close to 1/60th as possible
//

namespace audio {

// A top level object for managing an audio subsystem. This object must outlive
// AudioComponents that reference it.
class AudioSystem {
 public:
  AudioSystem(SampleRate sample_rate);

  // Should include builder style playback options
  /*


  util::Status CreateSampler(std::string_view resource_id);

  // For polyphony
  util::Status CreateMultiSampler(std::string_view resource_id);

  // has override methods for tempo/volume/pan
  util::Status CreateSequence(std::string_view resource_id);
  */

  void set_oscillator_rate(double rate);
  // Get a value from a system wide oscillator with oscillations between -1 and
  // 1. Values retrieved from this method should approximate a sinusoid.
  double GetOscillatorValue(uint32_t elapsed_samples);
  
  SampleRate sample_rate() const { return sample_rate_; }
 private:
  const SampleRate sample_rate_;
  // An oscillator rate in Cycles / Sample.
  double oscillator_rate_;
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