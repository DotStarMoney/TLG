#ifndef AUDIO_UTILS_H_
#define AUDIO_UTILS_H_

namespace audio {

// Given an acoustic gain in decibels, produce a value by which an audio signal
// could be multiplied to produce the amplitude change.
double DecibelsToMultiplier(double decibels);

// Given a semitone offset, compute a frequency multiplier that would produce
// the semitone offset.
double SemitoneShiftToFrequencyMultiplier(double semitone_shift);

} // namespace audio
 
#endif // AUDIO_UTILS_H_