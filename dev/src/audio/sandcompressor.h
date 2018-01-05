#ifndef AUDIO_SANDCOMPRESSOR_H_
#define AUDIO_SANDCOMPRESSOR_H_

#include "audio_format.h"
#include "samplesupplier.h"
#include "status.h"

namespace audio {

class SandCompressor : public SampleSupplier<double> {
 public:
  SandCompressor(SampleRate sample_rate);

  double attack() const;
  double release() const;
  double lookahead() const;

  double pre_gain() const;
  double post_gain() const;
  
  double threshold() const;
  double ratio() const;
  double knee() const;

};

} // namespace audio


#endif // AUDIO_SANDCOMPRESSOR_H_