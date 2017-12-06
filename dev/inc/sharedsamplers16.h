#ifndef AUDIO_SHAREDSAMPLERS16_H_
#define AUDIO_SHAREDSAMPLERS16_H_

#include <atomic>

#include "instrument_characteristics.h"
#include "samplers16.h"
#include "samplesupplier.h"
#include "audiosystem.h"
#include "audiocontext.h"
#include "zsequence.h"

// TODO(?): explain barrier resolution
// TODO(?): explain frames

namespace audio {

class SharedSamplerS16 : public AudioComponent,
    public SampleSupplier<int16_t> {
 public:
  explicit SharedSamplerS16(const AudioSystem* parent);
  ~SharedSamplerS16();

  typedef int64_t Token;

  // Set the pointer from which the playback base state will be copied out of
  // during barrier resolution if articulate_as_base_state_ is set. A sequence
  // can override audio set to trigger as a sampler base state
  //
  // Returns a token that should be passed into the next invocation of
  // PlayAsBaseState. This token is used to determine if calls to Disarm should
  // actually disarm the sampler.
  Token PlaySoundAsBaseState(Token prev_token,
      const AudioContext::Sound* base_state);

  // If the last played audio carried the provided token, stop and completely
  // disarm the sampler, used when audio objects pointing to this shared
  // sampler go out of scope or an audio object changes samples/instruments.
  void DisarmBaseState(Token prev_token);


  util::Status ProvideNextSamples(
      Iter samples_start,
      uint32_t sample_size,
      uint32_t sample_clock);

 private:
  // Set when an external audio object calls PlayAsBaseState, used to determine
  // if audio playback should be triggered as part of barrier resolution.
  bool articulate_as_base_state_;  

  SamplerS16 sampler_;

  SamplerS16::Parameters playback_params_;


};

} // namespace audio

#endif // AUDIO_SHAREDSAMPLERS16_H_