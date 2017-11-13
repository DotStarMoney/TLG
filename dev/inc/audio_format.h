#ifndef AUDIO_FORMAT_H_
#define AUDIO_FORMAT_H_

#include <string>

namespace audio {

// All audio formats are signed.
enum SampleFormat {
  INT8 = 0,
  INT16 = 1,
  INT32 = 2,
  INT64 = 3,
  FLOAT32 = 4,
  FLOAT64 = 5
};

enum ChannelLayout {
  MONO = 0,
  STEREO = 1
};

enum SampleRate : int {
  _32K = 32000,
  _44_1K = 44100
};

struct Format {
  SampleFormat sample_format;
  ChannelLayout layout;
  SampleRate sampling_rate;
  std::string ToString() const;
};

bool operator==(const Format& lhs, const Format& rhs);
bool operator!=(const Format& lhs, const Format& rhs);

} // namespace audio

#endif // AUDIO_FORMAT_H_