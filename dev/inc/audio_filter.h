#ifndef AUDIO_FILTER_H_
#define AUDIO_FILTER_H_

#include <array>

namespace audio {

// 2-mode streaming audio filter interface.
//
// Not thread safe.
class Filter {
 public:
  enum class Mode {
    kLowpass = 0,
    kHighpass = 1
  };
  Filter() : mode_(Mode::kLowpass) {} 
  virtual ~Filter() {}
 
  // Take the next sample in the stream, apply the filter, then return the
  // filtered value.
  virtual double Apply(double sample) = 0;

  void set_mode(Mode mode) { mode_ = mode; }
  Mode mode() const { return mode_; }
 protected:
  Mode mode_;
};

}

#endif // AUDIO_FILTER_H_