#ifndef AUDIO_AUDIOSYSTEM_H_
#define AUDIO_AUDIOSYSTEM_H_

#include <atomic>
#include <stdint.h>
#include <string_view>
#include <array>

#include "audio_format.h"
#include "audiocontext.h"
#include "status.h"
#include "stopwatch.h"
#include "outputqueue.h"
#include "samplers16.h"
#include "threadpool.h"

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
  static constexpr int kPolyphony = 8;
  AudioSystem(std::unique_ptr<OutputQueue> audio_queue,
      std::unique_ptr<util::Stopwatch> stopwatch);

  void SetContext(AudioContext* context);

  void Sync();

  void set_oscillator_rate(double rate);
  // Get a value from a system wide oscillator with oscillations between -1 and
  // 1. Values retrieved from this method should approximate a sinusoid.
  double GetOscillatorValue(uint32_t elapsed_samples);
  
  SampleRate sample_rate() const { return sample_rate_; }

  
 private:
  const SampleRate sample_rate_;
  // An oscillator rate in Cycles / Sample.
  double oscillator_rate_;

  std::atomic<AudioContext*> next_context_;
  AudioContext* current_context_;

  std::unique_ptr<OutputQueue> audio_queue_;
  std::unique_ptr<util::Stopwatch> stopwatch_;
};

// Child objects of an AudioSystem.
class AudioComponent {
 public:
  explicit AudioComponent(AudioSystem* const parent) : parent_(parent) {}
 protected:
  


  AudioSystem* const parent_; // Not owned.
};

} // namespace audio

#endif // AUDIO_AUDIOSYSTEM_H_