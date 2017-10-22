#include "stereosampler16.h"

#include <cmath>
#include <string>

#include "assert.h"
#include "status.h"

using ::util::StrCat;

namespace audio {
namespace {

// Utility Function Implementation Notes:
//
//     This is a case where we deliberately drop precision (float instead of
//     double) in the hopes that there is SOME audible nuance from doing so.
//

// 2^(1/12), which is the ratio between two frequencies seperated by a
// semitone in twelve-tone equal temperament. 
constexpr float kSemitoneFreqExpBase = 1.05946309436f;

// Given a semitone offset, compute a frequency multiplier for sample playback
// that would produce the semitone offset. The multiplier is to be used to
// multiply the rate of playback, so for semitones = 12., this will produce 2.,
// and so playback should occur at twice the sampler rate (or simulated to
// occur this way) to produce the semitone offset.
float SemitonesToFreqMul(float semitones) {
  return std::pow(kSemitoneFreqExpBase, semitones);
}

// Amount in percentage per sample that the previous value of
// StereoSampler16::playback_rate_smooth_ will contribute to its subsequent
// value. The remaining percentage is taken from the incoming playback rate.
constexpr float kPlaybackRatePortamento = 0.02f;
// Used to signify that StereoSampler16::playback_rate_smooth_ should be
// exactly set to an incoming playback rate.
constexpr float kInitialRateTarget = -1;

// True iff the provided pitch shift is something the sampler can use.
bool IsValidPitchShift(float pitch) {
  return std::isfinite(pitch);
}
// True iff the provided volume value is something the sampler can use.
bool IsValidVolume(float volume) {
  return (volume >= 0.0) && (volume <= 1.0);
}
} // namespace

// We've got a lot of parameters to initialize here, see the breakdown:
StereoSampler16::StereoSampler16(AudioSystem* const parent, int sample_rate) :
    AudioComponent(parent), SampleSupplier({INT16, STEREO, sample_rate}), 
    state_(kStopped),               // we aren't playing anything...
    status_(util::OkStatus),        // no issues yet...
    pan_(0.0),                      // pan to the middle
    pitch_shift_(0.0),              // pitch shift to 0 semitones
    volume_(0.5),                   // no volume change
    vibrato_range_(0.0),            // vibrato range to 0 semitones
    playback_pitch_shift_(0.0),     // this will be set whenever Play is
                                    //   called; any default will do
    playback_volume_(0.5),          // no volume change.
    playback_position_(0.0),        // set the playback cursor to 0
    playback_elapsed_samples_(0),   // no elapsed samples have gone by, so
                                    //   elapsed samples is 0
    playback_release_samples_(0),   // we only set the elapsed release time if
                                    //   we call Release, and we only care
                                    //   about the elapsed release time if we
                                    //   call Release, so we default it to 0
                                    //   since its initial value won't matter
    releasing_(false),              // we aren't releasing yet
    sample_(nullptr),               // we aren't armed
    release_from_(0.0),             // technically our volume envelope starts 
                                    //   at 0%, so we start release_from_ there
                                    //   as well
    playback_rate_smooth_(          // no sample playback yet, so we snap the
        kInitialRateTarget) {}      //   playback rate to the incoming rate the
                                    //   first time we see one.
    
int16_t StereoSampler16::GetSampleByIndex(
    const MonoSampleData16::SampleData& data, uint32_t index) const {
  if (index >= data.size()) return 0;
  return data[index];
}

double StereoSampler16::IntegrateWindowSlice(
    const MonoSampleData16::SampleData& data, double window_start,
    double window_end) const {
  const uint32_t sample_index = static_cast<uint32_t>(window_start);
  const int16_t s0 = GetSampleByIndex(data, sample_index);
  const int16_t s1 = GetSampleByIndex(data, sample_index + 1);
  
  // Get window bounds between 0 and 1.
  window_start -= sample_index;
  window_end -= sample_index;

  // This formula is an algebraic simplification of getting the linearly
  // interpolated sample values, then integrating over the curve that they
  // form.
  return 0.5 * (s0 + s1) * (window_end - window_start);
}

double StereoSampler16::IntegratePiecewiseLinearSamples(
    const MonoSampleData16::SampleData& data, double window_start, 
    double window_end) const {
  uint32_t snap_start = static_cast<uint32_t>(window_start);
  const uint32_t first_snap_end = snap_start + 1;
  // A little time saver in the common case for small windows
  if (window_end <= first_snap_end) {
    return IntegrateWindowSlice(data, window_start, window_end);
  }

  // Get the beginning "cap"
  double acc = IntegrateWindowSlice(data, window_start, first_snap_end);
  // Add in the area of each whole sample in between the window "caps"
  while ((++snap_start + 1) < window_end) {
    acc += IntegrateWindowSlice(data, snap_start, snap_start + 1);
  }
  // Add the ending "cap" to get the total area. This can sometimes be 0 when
  // the window end is exactly on a sample index (integral value).
  return acc + IntegrateWindowSlice(data, snap_start, window_end);
}

std::pair<double, bool> StereoSampler16::GetEnvelopeValue(
    double elapsed_samples) {
  if (releasing_) {
    const double rate_of_release = sample_->envelope().sustain / 
        sample_->envelope().release;
    const double release_value = release_from_ - elapsed_samples * 
        rate_of_release;
    if (release_value <= 0) return {0, false};
    return {release_value, true};
  }

  if (elapsed_samples < sample_->envelope().attack) {
    release_from_ = elapsed_samples / sample_->envelope().attack;
    return {release_from_, true};
  }
  elapsed_samples -= sample_->envelope().attack;
  if (elapsed_samples < sample_->envelope().decay) {
    release_from_ = 1 - (elapsed_samples / sample_->envelope().decay) *
        (1 - sample_->envelope().sustain);
    return {release_from_, true};
  }
  release_from_ = sample_->envelope().sustain;
  return {release_from_, true};
}

int16_t StereoSampler16::GetSample(double playback_position,
    double playback_rate) const {
  double playback_position_adj;
  double playback_rate_adj;
  // Scale the elapsed samples and playback rate should we be able to take
  // advantage of the frequency quadrupling pyramid.
  int level = 0;
  if (playback_rate > 4.0) {
    level = static_cast<int>(std::floor(std::log2(playback_rate)));
    if (level >= sample_->pyramid_levels()) {
      level = sample_->pyramid_levels() - 1;
    }
    double octave_shift = std::pow(4.0, level);
    playback_position_adj = playback_position / octave_shift;
    playback_rate_adj = playback_rate / octave_shift;
  }

  double window_start = playback_position_adj;
  double window_end;
  double sample_value = 0;

  if (sample_->has_loop()) {
    double loop_begin = sample_->loop_begin(level);
    double loop_length = sample_->loop_length(level);
    if (window_start > loop_begin) {
      window_start = std::fmod(window_start - loop_begin, loop_length) +
          loop_begin;
    }
    window_end = window_start + playback_rate_adj;
    double loop_end = loop_begin + loop_length;
    if ((window_end - window_start) > loop_length) {
      // In the weird case that the window is larger than the loop, MAKE the
      // window the loop
      window_start = loop_begin;
      window_end = loop_end;
    } else {
      // If the window exceeds the loop end, accumulate the wrap-around bit of
      // the window
      if (window_end > loop_end) {
        sample_value += IntegratePiecewiseLinearSamples(sample_->data(level), 
            loop_begin, window_end - loop_end);
        // Finally crop the window end to the loop end
        window_end = loop_end;
      }
    }
  } else {
    window_end = window_start + playback_rate_adj;
  }
  sample_value += IntegratePiecewiseLinearSamples(sample_->data(level), 
      window_start, window_end);

  return static_cast<int16_t>(sample_value / playback_rate_adj);
}

int16_t StereoSampler16::IterateNextSample(double playback_rate) {
  if (state_ != kPlaying) {
    return 0;
  }
  if (playback_rate_smooth_ == kInitialRateTarget) {
    playback_rate_smooth_ = playback_rate;
  } else {
    playback_rate_smooth_ = (playback_rate - playback_rate_smooth_) * 
        (1.0 - kPlaybackRatePortamento) + playback_rate_smooth_;
  }

  std::pair<double, bool> env = GetEnvelopeValue(playback_elapsed_samples_);
  int16_t val = GetSample(playback_position_, playback_rate_smooth_);
  
  // Advance both the elapsed samples since playback started and the cursor to
  // the sample data.
  ++playback_elapsed_samples_; 
  playback_position_ += playback_rate_smooth_;

  state_ = env.second ? kPlaying : kStopped;
  // If we don't loop, we might also have finished playing if we've run out of
  // samples to play.
  if (!sample_->has_loop() && (state_ == kPlaying)) {
    state_ = 
        (playback_position_ < sample_->data(0).size()) ? kPlaying : kStopped;
  }
  // The returned sample value is scaled by the volume envelope.
  return static_cast<int16_t>(std::round(val * env.first));
}

util::Status StereoSampler16::ProvideNextSamples(
    Iter samples_start, uint32_t sample_size, uint32_t sample_clock) {
 
  if (state_ != kPlaying) {
    // We provide stereo samples, so return 2 pairs of zeros for each sample.
    std::memset(&(*samples_start), 0, sample_size * 2);
    return util::OkStatus;
  }
  
  const double volume_multiplier = playback_volume_ + volume_;
  const float semitone_offset = playback_pitch_shift_ + pitch_shift_;
  const float pan_percent_r = (pan_ + 1.0f) * 0.5f;
  while (--sample_size >= 0) {
    int16_t sample = 0;
    const float final_offset = semitone_offset + 
        parent_->GetOscillatorValue(sample_clock) * vibrato_range_;

    // The failure mode for exceeding above the octaves covered by the pyramid 
    // is to return silence (each pyramid level covers 2 octaves / 24 
    // semitones).
    if (final_offset <= (sample_->pyramid_levels() * 24)) {
      sample = IterateNextSample(SemitonesToFreqMul(final_offset));
    }

    sample = static_cast<int16_t>(std::clamp(static_cast<double>(sample) * 
        volume_multiplier, -32768.0, 32767.0));

    *(samples_start++) = static_cast<int16_t>(sample * (1.0 - pan_percent_r));
    *(samples_start++) = static_cast<int16_t>(sample * pan_percent_r);
    ++sample_clock;
  }
  return util::OkStatus;
}

void StereoSampler16::Arm(const MonoSampleData16* sample) {
  Stop();
  sample_ = sample;
  if (sample == nullptr) return;
  ASSERT_EQ(sample->format(),
      audio::Format({INT16, MONO, format_.sampling_rate}));
}

void StereoSampler16::Stop() {
  state_ = kStopped;
  playback_position_ = 0.0;
  playback_elapsed_samples_ = 0;
  releasing_ = false;
  release_from_ = 0.0;
}

void StereoSampler16::Play(float semitone_shift, float volume) {
  // Since we check for any malformed input parameters to the sampler, we can
  // assume we're okay at this point.
  status_ = util::OkStatus;

  if (!IsValidPitchShift(semitone_shift)) {
    status_ = util::InvalidArgumentError(
      "Provided playback pitch shift is not finite.");
    return;
  }
  
  if (!IsValidVolume(volume)) {
    status_ = util::InvalidArgumentError(
      StrCat("Playback volume parameter must be in range [0.0, 1.0], got ",
        volume, "."));
    return;
  }

  if (sample_ == nullptr) return;
  
  if (state_ == kPaused) {
    state_ = kPlaying;
    return;
  }

  Stop();
  state_ = kPlaying;
  playback_pitch_shift_ = semitone_shift;
  playback_volume_ = volume_;
  
}

void StereoSampler16::Pause() {
  if (state_ != kPlaying) return;
  state_ = kPaused;
}

void StereoSampler16::Release() {
  if ((state_ != kPlaying) || releasing_) return;
  releasing_ = true;
  playback_release_samples_ = playback_elapsed_samples_;
}

void StereoSampler16::SetPan(float pan) {
  if ((pan < 1.0) || (pan > 1.0)) {
    status_ = util::InvalidArgumentError(
        StrCat("Panning parameter must be in range [-1.0, 1.0], got ", 
            pan, "."));
    return;
  }
  pan_ = pan;
}

void StereoSampler16::SetPitchShift(float semitone_shift) {
  if (!IsValidPitchShift(semitone_shift)) {
    status_ = util::InvalidArgumentError(
        "Provided pitch shift is not finite.");
    return;
  }
  pitch_shift_ = semitone_shift;
}

void StereoSampler16::SetVolume(float volume) {
  if (!IsValidVolume(volume)) {
    status_ = util::InvalidArgumentError(
      StrCat("Volume parameter must be in range [0.0, 1.0], got ",
        volume, "."));
    return;
  }
  volume_ = volume;
}

void StereoSampler16::SetVibratoRange(float range_semitones) {
  if (!IsValidPitchShift(range_semitones)) {
    status_ = util::InvalidArgumentError(
      "Provided vibrato range is not finite.");
    return;
  }
  vibrato_range_ = range_semitones;
}

} // namespace audio