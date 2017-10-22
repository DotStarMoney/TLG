#ifndef AUDIO_STEROSAMPLER16_H_
#define AUDIO_STEROSAMPLER16_H_

#include <cstdint>
#include <memory>

#include "audiosystem.h"
#include "fixed_point.h"
#include "monosampledata16.h"
#include "samplesupplier.h"
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
class StereoSampler16 : public AudioComponent, public SampleSupplier<int16_t> {
 public:
  explicit StereoSampler16(AudioSystem* const parent, int sample_rate);

  enum State {
    kStopped = 0,
    kPaused = 1,
    kPlaying = 2
  };

  // Call Stop and prepare a sample for playback. Playing a sample when
  // disarmed is not an error and results in silence. Arming a nullptr will
  // default to playing silence.
  void Arm(const MonoSampleData16* sample);

  // Stop playback and prepare the sampler for another playback. 
  void Stop();

  // If stopped or playing: start a new playback.
  // If paused: resume playback.
  void Play(float semitone_shift = 0.0, float volume_ = 0.5);
  
  // Pause any on-going playback.
  void Pause();
  
  // Start releasing a sample if its playing or paused.
  void Release();

  // Sets playback panning. [pan] is from -1 to 1, corresponding to an all the
  // way left, to an all the way right panning.
  void SetPan(float pan);

  // Sets the playback pitch shift in semitones. [semitone_shift] must be
  // finite.
  void SetPitchShift(float semitone_shift);

  // Sets the volume mapping value. [volume] is from 0 to 1, corresponding to
  //(0) silence, (0.5) no volume change, and (1) twice the amplitude.
  void SetVolume(float volume);

  // Sets the vibrato range in semitones. [range_semitones] must be finite.
  void SetVibratoRange(float range_semitones);

  State state() const { return state_; }
  bool armed() const { return sample_ != nullptr; }
  float pan() const { return pan_; }
  float pitch_shift() const { return pitch_shift_; }
  float volume() const { return volume_; }
  float vibrato_range() const { return vibrato_range_; }
  const util::Status& status() const { return status_; }

 protected:

  util::Status ProvideNextSamples(
    Iter samples_start,
    uint32_t sample_size,
    uint32_t sample_clock) override;

 private:

  State state_;

  // Operation of the sampler is undefined when status_ is not ok.
  util::Status status_;

  // Theres no real reason to use floats over doubles here other than floats
  // are less precise and "arguably" could contribue to interesting sound
  // characteristics of sample playback.
  //
  // L to R panning between -1 and 1
  float pan_;
  // Pitch shift in semitones.
  float pitch_shift_;
  // A value between 0 and 1 (added to playback_volume_) used to produce a
  // volume mapping for each sample. Samples are mapped by adding volume_ to
  // playback_volume_ then multiplying this value with the sample. In this way,
  // there is no volume change when volume_ and playback_volume_ = 0.5, and
  // the volume is doubled when both are set to 1.0.
  float volume_;
  // Vibrato range in semitones.
  float vibrato_range_;

  // A pitch shift in semitones that is the base pitch offset used to compute
  // a playback rate for sample playback.
  float playback_pitch_shift_;
  // Added to volume_ and optionally set on each playback. See docs for volume_
  // for mapping details.
  float playback_volume_;
  // The position in samples of the playback cursor. It is this that is
  // actually advanced by the playback rate.
  double playback_position_;
  // The elapsed samples since playback began, obviously tied to our sampling
  // rate.
  uint32_t playback_elapsed_samples_;
  // The elapsed samples since playback began of when this sample began its
  // release.
  uint32_t playback_release_samples_;
  // True if this sample is releasing, false otherwise.
  bool releasing_;

  // A reference to the sample data we play, nullptr if we don't have a sample.
  //
  // Not owned.
  const MonoSampleData16* sample_;
  
  // Get a sample by its index, returning 0 for samples outside the range of
  // the provided data.
  int16_t GetSampleByIndex(const MonoSampleData16::SampleData& data, 
      uint32_t index) const;

  // A method that, given a slice of a sampling window confined to a single
  // sample, returns the integral of the linearly interpolated sample data
  // over the window slice.
  double IntegrateWindowSlice(const MonoSampleData16::SampleData& data,
      double window_start, double window_end) const;

  // Treating data as a piecewise linear curve with each sample 1.0 units from
  // the previous, integrate over the window.
  double IntegratePiecewiseLinearSamples(
      const MonoSampleData16::SampleData& data, double window_start,
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

  // Given the current sample playback position, get a linearly interpolated
  // sample given the playback rate.
  int16_t GetSample(double playback_position, double playback_rate) const;

  // Given a playback rate, return the next sample this sampler would emit
  // while also updating the state. This will set playing_ = false when sample
  // playback has ended based on the current configuration.
  int16_t IterateNextSample(double playback_rate);
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
 
#endif // AUDIO_STEREOSAMPLER16_H_