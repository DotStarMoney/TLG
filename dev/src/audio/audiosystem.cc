#include "audiosystem.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace audio {
namespace {

constexpr double kOscillatorRateHz = 8.0;

} // namespace

AudioSystem::AudioSystem(SampleRate sample_rate) : sample_rate_(sample_rate) {
  set_oscillator_rate(kOscillatorRateHz);

}

void AudioSystem::set_oscillator_rate(double rate) {
  oscillator_rate_ = rate / sample_rate_;
}

double AudioSystem::GetOscillatorValue(uint32_t elapsed_samples) {
  return static_cast<double>(
      sin(elapsed_samples * oscillator_rate_ * M_PI * 2));
}

void AudioSystem::SetContext(AudioContext* context) {
  next_context_.store(context);
}

void AudioSystem::Sync() {

  double elapsed_secs = stopwatch_->Lap();
  
  

  current_context_ = next_context_;
}


} // namespace audio