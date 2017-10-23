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
  
  const Format& format() { return format_; }
 protected:
  explicit SampleSupplier(Format format) : format_(format) {}

  Format format_;
};

} // namespace audio

#endif // AUDIO_SAMPLESUPPLIER_H_