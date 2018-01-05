#include "samplers16.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "assert.h"
#include "audio_defaults.h"
#include "status.h"
#include "audio_utils.h"

using ::util::StrCat;

namespace audio {
namespace {
// Amount in percentage per sample that the previous value of
// StereoSampler16::playback_rate_smooth_ will contribute to its subsequent
// value. The remaining percentage is taken from the incoming playback rate.
constexpr float kPlaybackRatePortamento = 0.02f;
// Used to signify that StereoSampler16::playback_rate_smooth_ should be
// exactly set to an incoming playback rate.
constexpr float kInitialRateTarget = -1;

InstrumentCharacteristics::ADSRSamples ADSRSecondsToSamples(
    const InstrumentCharacteristics::ADSRSeconds& adsr_seconds,
    SampleRate rate) {
  return {adsr_seconds.attack * rate, 
      adsr_seconds.decay * rate, 
      adsr_seconds.sustain, 
      adsr_seconds.release * rate};
}
} // namespace
    
// We've got a lot of parameters to initialize here, see the breakdown:
SamplerS16::SamplerS16(AudioSystem* const parent) :
    AudioComponent(parent),
    SampleSupplier({INT16, STEREO, parent_->sample_rate()}), 
    state_(kStopped),               // we aren't playing anything...
    playback_parameters_(nullptr),  // no playback parameters yet
    playback_pitch_shift_(0.0),     // this will be set whenever Play is
                                    //   called; any default will do
    playback_volume_(kVolume100P),  // no volume change.
    playback_position_(0.0),        // set the playback cursor to 0
    playback_elapsed_samples_(0),   // no elapsed samples have gone by, so
                                    //   elapsed samples is 0
    playback_released_samples_(0),  // we only set the elapsed release time if
                                    //   we call Release, and we only care
                                    //   about the elapsed release time if we
                                    //   call Release, so we default it to 0
                                    //   since its initial value won't matter
    releasing_(false),              // we aren't releasing yet
    sample_(nullptr),               // we aren't armed
    envelope_(&default_envelope),   // start with the default envelope
                                    // start with the default loop info
    loop_info_levels_(&default_loop_info),
                                    // it doesn't really matter how we set this
                                    //   as we'l always update update it before
                                    //   we use it
    converted_envelope_(InstrumentCharacteristics::DefaultEnvelope),
                                    // see comment for converted_envelope_
                                    //   above
    converted_loop_info_(
        {InstrumentCharacteristics::DefaultLoopInfo.mode,{}}),
                                    // initialize the default instrument
                                    //   characteristics
    default_loop_info(
        {InstrumentCharacteristics::DefaultLoopInfo.mode, {}}),
    default_envelope(
        ADSRSecondsToSamples(
            InstrumentCharacteristics::DefaultEnvelope, 
            format_.sampling_rate)),
    release_from_(0.0),             // technically our volume envelope starts 
                                    //   at 0%, so we start release_from_ there
                                    //   as well
    playback_rate_smooth_(          // no sample playback yet, so we snap the
        kInitialRateTarget) {}      //   playback rate to the incoming rate the
                                    //   first time we see one.
    
int16_t SamplerS16::GetSampleByIndex(
    const SampleDataM16::SampleData& data, uint32_t index) const {
  if (index >= data.size()) return 0;
  return data[index];
}

double SamplerS16::IntegrateWindowSlice(
    const SampleDataM16::SampleData& data, double window_start,
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

double SamplerS16::IntegratePiecewiseLinearSamples(
    const SampleDataM16::SampleData& data, double window_start,
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

std::pair<double, bool> SamplerS16::GetEnvelopeValue(
    double elapsed_samples) {
  if (releasing_) {
    const double rate_of_release = envelope_->sustain / envelope_->release;
    const double release_value = release_from_ -
        ((elapsed_samples - playback_released_samples_) * rate_of_release);
    if (release_value <= 0) return {0, false};
    return {release_value, true};
  }

  if (elapsed_samples < envelope_->attack) {
    release_from_ = elapsed_samples / envelope_->attack;
    return {release_from_, true};
  }

  elapsed_samples -= envelope_->attack;
  if (elapsed_samples < envelope_->decay) {
    release_from_ = 1 - (elapsed_samples / envelope_->decay) *
        (1 - envelope_->sustain);
    return {release_from_, true};
  }
  
  release_from_ = envelope_->sustain;
  return {release_from_, true};
}

int16_t SamplerS16::GetSample(double playback_position,
    double playback_rate) const {
  double playback_position_adj = playback_position;
  double playback_rate_adj = playback_rate;
  // Scale the elapsed samples and playback rate should we be able to take
  // advantage of the frequency quadrupling pyramid.
  int level = 0;
  if (playback_rate > 4.0) {
    // log2(x) / 2 == log4(x)
    level = static_cast<int>(std::floor(std::log2(playback_rate) * 0.5));
    // We don't allow playback in frequencies above what is contained in the
    // highest frequency pyramid level, so we don't need to bounds check level
    // for safe future array accesses.
    const double octave_shift = std::pow(4.0, level);
    playback_position_adj = playback_position / octave_shift;
    playback_rate_adj = playback_rate / octave_shift;
  }

  double window_start = playback_position_adj;
  double window_end;
  double sample_value = 0;

  if (loop_info_levels_->mode == InstrumentCharacteristics::LoopMode::LOOP) {
    // These array accesses are safe as we guarentee in the Arm* functions
    // that IF we can play a sample with pyramid levels, the loop bounds levels
    // match.
    const double loop_begin = loop_info_levels_->bounds_levels[level].begin;
    const double loop_length = loop_info_levels_->bounds_levels[level].length;
    if (window_start > loop_begin) {
      window_start = std::fmod(window_start - loop_begin, loop_length) +
          loop_begin;
    }
    window_end = window_start + playback_rate_adj;
    const double loop_end = loop_begin + loop_length;
    if ((window_end - window_start) > loop_length) {
      // In the weird case that the window is larger than the loop, MAKE the
      // window the loop
      window_start = loop_begin;
      window_end = loop_end;
    } else if (window_end > loop_end) {
      // If the window exceeds the loop end, accumulate the wrap-around bit of
      // the window
      sample_value += IntegratePiecewiseLinearSamples(sample_->data(level), 
          loop_begin, loop_begin + (window_end - loop_end));
      // Finally crop the window end to the loop end
      window_end = loop_end;
    }
  } else {
    window_end = window_start + playback_rate_adj;
  }
  sample_value += IntegratePiecewiseLinearSamples(sample_->data(level), 
      window_start, window_end);

  return static_cast<int16_t>(sample_value / playback_rate_adj);
}

int16_t SamplerS16::IterateNextSample(float semitone_shift) {
  if (state_ != kPlaying) {
    return 0;
  }

  const float playback_rate =
      SemitoneShiftToFrequencyMultiplier(semitone_shift);
  if (playback_rate_smooth_ == kInitialRateTarget) {
    playback_rate_smooth_ = playback_rate;
  }
  else {
    playback_rate_smooth_ = (playback_rate - playback_rate_smooth_) *
      (1.0 - kPlaybackRatePortamento) + playback_rate_smooth_;
  }

  int16_t val = 0;
  // Getting interpolated samples at high frequencies is expensive, so we limit
  // the maximum frequency to that which requires at most 4 samples of the
  // source sample data (x4 so two octaves or 24 semitones).
  if (semitone_shift < (sample_->pyramid_levels() * 24)) {
    val = GetSample(playback_position_, playback_rate_smooth_);
  }

  std::pair<double, bool> env = GetEnvelopeValue(playback_elapsed_samples_);
  
  // Advance both the elapsed samples since playback started and the cursor to
  // the sample data.
  ++playback_elapsed_samples_; 
  playback_position_ += playback_rate_smooth_;

  state_ = env.second ? kPlaying : kStopped;
  // If we don't loop, we might also have finished playing if we've run out of
  // samples to play.
  if (!(loop_info_levels_->mode == 
      InstrumentCharacteristics::LoopMode::LOOP) && (state_ == kPlaying)) {
    state_ = 
        (playback_position_ < sample_->data(0).size()) ? kPlaying : kStopped;
  }
  // The returned sample value is scaled by the volume envelope.
  return static_cast<int16_t>(std::round(val * env.first));
}

util::Status SamplerS16::ProvideNextSamples(
    Iter samples_start, uint32_t sample_size, uint32_t sample_clock) {
 
  if (state_ != kPlaying) {
    // We provide stereo samples, so return 2 pairs of zeros for each sample.
    std::memset(&(*samples_start), 0, sample_size * 2);
    return util::OkStatus;
  }
  
  const double volume_multiplier = playback_volume_ * 
      playback_parameters_->volume;
  const float semitone_offset = playback_pitch_shift_ + 
      playback_parameters_->pitch_shift;
  const float pan_percent_r = (playback_parameters_->pan + 1.0f) * 0.5f;
  do {
    int16_t sample = 0;
    const double final_offset = semitone_offset + 
        parent_->GetOscillatorValue(sample_clock) * playback_parameters_->vibrato_range;

    sample = IterateNextSample(static_cast<float>(final_offset));
    
    sample = static_cast<int16_t>(std::round(std::clamp(
        static_cast<double>(sample) * volume_multiplier, -32768.0, 32767.0)));

    *(samples_start++) = static_cast<int16_t>(sample * (1.0 - pan_percent_r));
    *(samples_start++) = static_cast<int16_t>(sample * pan_percent_r);
    ++sample_clock;
  } while (sample_size-- > 1);
  return util::OkStatus;
}

void SamplerS16::ExpandLoopInfo() {
  // We don't worry about the loop bounds if theres no sample from which to
  // extract the pyramid levels, if this is a one shot sample, or if we're
  // currently pointing at the default loop info (which is one shot).
  if ((sample_ == nullptr) || 
      (converted_loop_info_.mode == 
          InstrumentCharacteristics::LoopMode::ONE_SHOT) || 
      (loop_info_levels_ == &default_loop_info)) return;

  // Shorten the values of the loop bounds in successive levels to reflect
  // how the loop bounds change with the similarly quartered sample data when
  // the sample data is in a pyramid.
  converted_loop_info_.bounds_levels.resize(sample_->pyramid_levels());
  for (unsigned int level = 1;
      level < converted_loop_info_.bounds_levels.size();
      ++level) {
    converted_loop_info_.bounds_levels[level].begin = 
        converted_loop_info_.bounds_levels[level - 1].begin * 0.25;
    converted_loop_info_.bounds_levels[level].length =
        converted_loop_info_.bounds_levels[level - 1].length * 0.25;
  }
}

void SamplerS16::ArmSample(const SampleDataM16* sample) {
  ASSERT_EQ(state_, kStopped);
  sample_ = sample;
  if (sample == nullptr) return;
  ASSERT_EQ(sample->format(),
      audio::Format({INT16, MONO, format_.sampling_rate}));
  // When we have some loop information, but its incomplete as we don't yet
  // have a sample from which to extract the pyramid levels, we'll need to
  // expand the loop info here as we might have pyramid levels.
  ExpandLoopInfo();
}

void SamplerS16::ArmParameters(const Parameters* parameters) {
  ASSERT_EQ(state_, kStopped);
  playback_parameters_ = parameters;
}

void SamplerS16::ArmLoop(
    const InstrumentCharacteristics::LoopInfo* loop_info) {
  ASSERT_EQ(state_, kStopped);
  if (loop_info == nullptr) {
    loop_info_levels_ = &default_loop_info;
    return;
  }
  converted_loop_info_ = {loop_info->mode, {loop_info->bounds}};
  ExpandLoopInfo();
  loop_info_levels_ = &converted_loop_info_;
}

void SamplerS16::ArmEnvelope(
    const InstrumentCharacteristics::ADSRSeconds* envelope) {
  ASSERT_EQ(state_, kStopped);
  if (envelope == nullptr) {
    envelope_ = &default_envelope;
    return;
  }
  converted_envelope_ = ADSRSecondsToSamples(*envelope, format_.sampling_rate);
  envelope_ = &converted_envelope_;
}

void SamplerS16::Stop() {
  state_ = kStopped;
  playback_position_ = 0.0;
  playback_elapsed_samples_ = 0;
  releasing_ = false;
  release_from_ = 0.0;
}

void SamplerS16::Play(float semitone_shift, float volume) {
  ASSERT_NE(playback_parameters_, nullptr);
  ASSERT_NE(sample_, nullptr);
  ASSERT_EQ(state_, kStopped);

  if (state_ == kPaused) {
    state_ = kPlaying;
    return;
  }

  state_ = kPlaying;
  playback_pitch_shift_ = semitone_shift;
  playback_volume_ = volume;
}

void SamplerS16::Pause() {
  if (state_ != kPlaying) return;
  state_ = kPaused;
}

void SamplerS16::Release() {
  if ((state_ != kPlaying) || releasing_) return;
  releasing_ = true;
  playback_released_samples_ = playback_elapsed_samples_;
}

} // namespace audio