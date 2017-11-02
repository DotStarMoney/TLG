#ifndef AUDIO_STEREOSEQUENCER16_H_
#define AUDIO_STEREOSEQUENCER16_H_

#include "audiosystem.h"
#include "samplesupplier.h"

namespace audio {

class StereoSequencer16 : public AudioComponent, 
    public SampleSupplier<int16_t> {
  public:
    explicit StereoSequencer16(AudioSystem* const parent);

    // Arm() with some sort of sequence data object
    // ArmBank() with some sort of bank object?

    util::Status ProvideNextSamples(
      Iter samples_start,
      uint32_t sample_size,
      uint32_t sample_clock) override;
};



} // namespace


#endif // AUDIO_STEREOSEQUENCER16_H_