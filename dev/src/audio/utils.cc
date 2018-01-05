#include "audio/utils.h"

#include <math.h>

namespace audio {

double DecibelsToMultiplier(double decibels) {
  return pow(10.0, decibels / 20.0);
}

// 2^(1/12), which is the ratio between two frequencies seperated by a
// semitone in twelve-tone equal temperament. 
constexpr float kSemitoneFreqExpBase = 1.05946309436f;

double SemitoneShiftToFrequencyMultiplier(double semitone_shift) {
  return pow(kSemitoneFreqExpBase, semitone_shift);
}

} // namespace audio