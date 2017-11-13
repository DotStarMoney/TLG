#ifndef AUDIO_SAMPLESUPPLIER_H_
#define AUDIO_SAMPLESUPPLIER_H_

#include <vector>

#include "audio_format.h"
#include "status.h"

// A SampleSupplier provides an interface for lazy evaluation of objects that
// can produce samples. An object implementing SampleSupplier can itself have
// dependent SampleSuppliers, such that a call to ProvideNextSamples for a top
// level supplier will propagate the request down through its dependencies.
//
// SampleSupplier builds the sample rate and channel type into the object so 
// that we can make smart decisions about data we receive when requesting
// samples. More often than not, if a dependency provides samples at a sampling
// rate or channel type not matching ours, this is an error.

namespace audio {

template <class SampleT>
class SampleSupplier {
 public:
  typedef typename std::vector<SampleT>::iterator Iter;
  virtual util::Status ProvideNextSamples(
    Iter samples_start,
    uint32_t sample_size,
    uint32_t sample_clock) = 0;
  
  const Format& format() const { return format_; }
 protected:
  explicit SampleSupplier(Format format) : format_(format) {}

  Format format_;
};

// A sample supplier with multiple outputs.
//
// **IMPORTANT**
// channels_ must remain fixed throughout any calls to MultiSampleSupplier
// until all channels have supplied their samples. This way, a user can check
// the number of channels, then combine the audio from each channel without
// worrying about accessing a non-existant channel (since their number is
// fixed through the duration of the calls)
//
template <class SampleT>
class MultiSampleSupplier {
public:
  typedef typename std::vector<SampleT>::iterator Iter;
  virtual util::Status MultiProvideNextSamples(
    int channel,
    Iter samples_start,
    uint32_t sample_size,
    uint32_t sample_clock) = 0;

  const Format& format() const { return format_; }
  int channels() const { return channels_; }
protected:
  explicit MultiSampleSupplier(Format format) : format_(format), 
      channels_(0) {}

  Format format_;
  int channels_;
};

} // namespace audio

#endif // AUDIO_SAMPLESUPPLIER_H_