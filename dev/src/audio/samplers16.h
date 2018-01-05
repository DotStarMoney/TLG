#ifndef AUDIO_SAMPLERS16_H_
#define AUDIO_SAMPLERS16_H_

#include <cstdint>
#include <memory>

#include "audiosystem.h"
#include "fixed_point.h"
#include "sampledatam16.h"
#include "samplesupplier.h"
#include "instrument_characteristics.h"
#include "status.h"

namespace audio {

// An old-sKo0l sampler fixed at stereo output of 16bit samples. This sampler
// is not designed for accurate sample playback so much as its designed to have
// interesting playback characteristics.
//
// This sampler is not thread-safe, and calls to any of the modifiers,
// ProvideNextSamples, and accessors must be externally synchronized.
//
// Semitones herein are used in the context of twelve-tone equal temperament.
class SamplerS16 : public AudioComponent, public SampleSupplier<int16_t> {
 public:
  explicit SamplerS16(AudioSystem* const parent);

  // Playback parameters provided externally.
  struct Parameters {
    Parameters() : pan(kPanMiddle), pitch_shift(0), volume(kVolume100P),
        vibrato_range(0) {}

    // L to R panning between -1 and 1
    float pan;
    // Pitch shift in semitones.
    float pitch_shift;
    // A value between 0 and 1 (multiplied with playback_volume_) used to
    // produce a volume mapping for each sample.
    float volume;
    // Vibrato range in semitones.
    float vibrato_range;
  };

  enum State {
    kStopped = 0,
    kPaused = 1,
    kPlaying = 2
  };

  // NOTE: Playback MUST be stopped before any Arm* method, or a call to Play
  // can take effect.
 
  // Set the sample data.
  void ArmSample(const SampleDataM16* sample);
  // Set the playback parameters. These have no default, and therefore must be
  // set before playback can take place.
  void ArmParameters(const Parameters* parameters);
  // Set the loop info. Arming a nullptr will default to using the
  // InstrumentCharacteristics default loop info.
  void ArmLoop(const InstrumentCharacteristics::LoopInfo* loop_info);
  // Set the envelope. Arming a nullptr will default to using the
  // InstrumentCharacteristics default envelope.
  void ArmEnvelope(const InstrumentCharacteristics::ADSRSeconds* envelope);

  // Stop playback and prepare the sampler for another playback. 
  void Stop();

  // If stopped: start a new playback.
  // If paused: resume playback at the previous semitone shift and volume, 
  // regardless of how they're set here.
  //
  // The volume provided here is multiplied with the parameter volume to
  // compute the per-sample volume multiplier.
  void Play(float semitone_shift = 0.0, float volume_ = 1.0);
  
  // Pause any on-going playback.
  void Pause();
  
  // Start releasing a sample if its playing or paused.
  void Release();

  State state() const { return state_; }

  util::Status ProvideNextSamples(
      Iter samples_start,
      uint32_t sample_size,
      uint32_t sample_clock) override;

 private:
  State state_;

  // The playback parameters.
  const Parameters* playback_parameters_;
  // A pitch shift in semitones that is the base pitch offset used to compute
  // a playback rate for sample playback.
  float playback_pitch_shift_;
  // Multiplied with volume_ and optionally set on each playback. See docs for 
  // volume_ for mapping details.
  float playback_volume_;
  // The position in samples of the playback cursor. It is this that is
  // actually advanced by the playback rate.
  double playback_position_;
  // The elapsed samples since playback began, obviously tied to our sampling
  // rate.
  uint32_t playback_elapsed_samples_;
  // The elapsed samples since playback began of when this sample began its
  // release.
  uint32_t playback_released_samples_;
  // True if this sample is releasing, false otherwise.
  bool releasing_;

  // A pointer to the sample data we play, nullptr if we don't have a sample.
  //
  // Not owned.
  const SampleDataM16* sample_;
  
  // A loop mode, paired with loop bounds adjusted for a frequency pyramid.
  struct LoopInfoLevels {
    InstrumentCharacteristics::LoopMode mode;
    std::vector<InstrumentCharacteristics::LoopBounds> bounds_levels;
  };
  // A pointer to the looping information used for sample playback.
  const LoopInfoLevels* loop_info_levels_;
  // A pointer to the ADSR envelope used for sample playback.
  const InstrumentCharacteristics::ADSRSamples* envelope_;

  // The converted envelope, and pyramided loop info. These are set when
  // ArmLoop or ArmEnvelope are called. We seperate these from the values used
  // in the sampler for playback so that switching to default values is as
  // fast as changing the pointer target to envelope_ and loop_info_levels_.
  InstrumentCharacteristics::ADSRSamples converted_envelope_;
  LoopInfoLevels converted_loop_info_;
  // If converted_loop_info_ is in fewer levels than sample_, expand
  // converted_loop_info_ so it reflects the pyramid quadrupling over the
  // sample data that the levels of the current sample do. If the sample is a 
  // nullptr, nothing happens.
  void ExpandLoopInfo();

  // Used as targets for the instrument characteristics when armed with a
  // nullptr. These aren't static as at least default_envelope varies with
  // the sampling rate.
  const LoopInfoLevels default_loop_info;
  const InstrumentCharacteristics::ADSRSamples default_envelope;

  // Get a sample by its index, returning 0 for samples outside the range of
  // the provided data.
  int16_t GetSampleByIndex(const SampleDataM16::SampleData& data,
      uint32_t index) const;

  // A method that, given a slice of a sampling window confined to a single
  // sample, returns the integral of the linearly interpolated sample data
  // over the window slice.
  double IntegrateWindowSlice(const SampleDataM16::SampleData& data,
      double window_start, double window_end) const;

  // Treating data as a piecewise linear curve with each sample 1.0 units from
  // the previous, integrate over the window.
  double IntegratePiecewiseLinearSamples(
      const SampleDataM16::SampleData& data, double window_start,
      double window_end) const;

  // Returns the volume percent specified by the envelope. This method uses the 
  // release time and the sustain to calculate a "rate of release." This is so
  // that, if the sample is released during a section of the envelope other
  // than the sustain, the release to zero is not too long/short and there is
  // not a spike of the volume percentage up/down to the sustain value.
  // This also returns false if the envelope has reached the end of the
  // release, true otherwise.
  std::pair<double, bool> GetEnvelopeValue(double elapsed_samples);
  // Used to track the value of the envelope before releasing began. Always
  // starts at 0.
  double release_from_;

  // At the current sample playback position, get a linearly interpolated
  // sample given the playback rate.
  int16_t GetSample(double playback_position, double playback_rate) const;

  // Given a semitone_shift, return the next sample this sampler would emit
  // while also updating the state. This will set state_ = kStopped when sample
  // playback has ended based on the current configuration.
  int16_t IterateNextSample(float semitone_shift);
  // The actual rate of playback. Instead of just using the desired playback
  // rate explicitly, we update a playback target on each active 
  // (state_ = kPlaying) invocation of IterateNextSample to slightly smooth the
  // transition between desired playback rates. This should add a bit of
  // portamento thats only noticable in big note value jumps (to emulate old
  // analog hardware characteristics).
  //
  // Defaults to a value that signifies it should be exactly set the first time
  // it sees a playback_rate
  double playback_rate_smooth_;
};
} // namespace audio
 
#endif // AUDIO_SAMPLERS16_H_