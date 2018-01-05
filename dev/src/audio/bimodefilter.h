#ifndef AUDIO_BIMODEFILTER_H_
#define AUDIO_BIMODEFILTER_H_

namespace audio {

// 2-mode streaming audio filter interface.
//
// Not thread safe.
class BiModeFilter {
 public:
  enum class Mode {
    kLowpass = 0,
    kHighpass = 1
  };
  BiModeFilter() : mode_(Mode::kLowpass) {} 
  virtual ~BiModeFilter() {}
 
  // Take the next sample in the stream, apply the filter, then return the
  // filtered value.
  virtual double Apply(double sample) = 0;

  void set_mode(Mode mode) { mode_ = mode; }
  Mode mode() const { return mode_; }
 protected:
  Mode mode_;
};

}

#endif // AUDIO_BIMODEFILTER_H_