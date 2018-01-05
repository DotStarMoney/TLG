#ifndef AUDIO_INSTRUMENT_CHARACTERISTICS_H_
#define AUDIO_INSTRUMENT_CHARACTERISTICS_H_

namespace audio {

class InstrumentCharacteristics {
 public:
  // Describes the looping playback characteristic of an instrument. ONE_SHOT 
  // describes an instrument that plays straight to the end regardles of how 
  // long the instrument is held. LOOP describes an instrument whose sample
  // data is played until the loop end point, then jumps back to the loop start
  // if instrument playback is still being "held" at the time it reaches the 
  // end of the loop and so on...
  enum class LoopMode {
    ONE_SHOT = 0,
    LOOP = 1
  };

  // If instrument playback should loop: these describe the point at which the 
  // loop starts, and the length of the loop in samples. Obviously, these
  // values should not exceed the number of samples in the underlying sample 
  // data.
  struct LoopBounds {
    double begin;
    double length;
  };
  struct LoopInfo {
    LoopMode mode;
    LoopBounds bounds;
  };

  // A time varying volume envelope describing how this instrument should be
  // played back.
  struct ADSRSeconds {
    // The time in seconds for a played sample to reach peak volume
    // starting from silence.
    float attack;
    // The time in seconds for a played sample, following the attack, to
    // reach the sustain volume.
    float decay;
    // A percentage of the sample volume at which a played sample is sustained
    // until released. 
    float sustain;
    // The time in seconds for a played sample, following playback release,
    // to reach silence from the current volume percentage.
    float release;
  };
  // Like ADSRSeconds, except time based values are in units of elapsed
  // samples.
  //
  // NOTE: These values are meaningless unless associated with a sample rate.
  typedef ADSRSeconds ADSRSamples;

  static constexpr LoopInfo DefaultLoopInfo{LoopMode::ONE_SHOT, {0.0, 0.0}};
  static constexpr ADSRSeconds DefaultEnvelope{0, 0, 1.0, 0};
};

}


#endif // AUDIO_INSTRUMENT_CHARACTERISTICS_H_