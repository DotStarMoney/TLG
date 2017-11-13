#ifndef AUDIO_SEQUENCERS16_H_
#define AUDIO_SEQUENCERS16_H

#include "audiosystem.h"
#include "samplesupplier.h"

// Think about priority samplers

namespace audio {

class SequencerS16 : public AudioComponent, 
    public MultiSampleSupplier<int16_t> {
 public:
  explicit SequencerS16(AudioSystem* const parent);

  void ArmSequence();
  void ArmSampler();
  void ArmBank();

  void Play();
  void Pause();
  void Stop();

  util::Status MultiProvideNextSamples(
      int channel,
      Iter samples_start,
      uint32_t sample_size,
      uint32_t sample_clock) override;
 
 private:

};


} // namespace audio


#endif // AUDIO_SEQUENCERS16_H__