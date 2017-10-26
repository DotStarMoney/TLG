#include "audiosystem.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace audio {
namespace {

constexpr float kOscillatorRateHz = 8.0;

} // namespace

AudioSystem::AudioSystem(SampleRate sample_rate) : sample_rate_(sample_rate) {
  set_oscillator_rate(kOscillatorRateHz);
}

void AudioSystem::set_oscillator_rate(float rate) {
  oscillator_rate_ = rate / sample_rate_;
}

float AudioSystem::GetOscillatorValue(uint32_t elapsed_samples) {
  return static_cast<float>(
      sin(elapsed_samples * oscillator_rate_ * M_PI * 2));
}

} // namespace audio