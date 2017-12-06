#include "audio_format.h"

#include "assert.h"
#include "tostring.h"
#include "strcat.h"

namespace audio {

bool operator==(const Format& lhs, const Format& rhs) {
  return (lhs.sample_format == rhs.sample_format) &&
    (lhs.layout == rhs.layout) && (lhs.sampling_rate == rhs.sampling_rate);
}
bool operator!=(const Format& lhs, const Format& rhs) {
  return (lhs.sample_format != rhs.sample_format) ||
    (lhs.layout != rhs.layout) || (lhs.sampling_rate != rhs.sampling_rate);
}

std::string Format::ToString() const {
  static const std::string format_strings[] = {"INT8", "INT16", "INT32",
      "INT64", "FLOAT32", "FLOAT64"};
  static const std::string layout_strings[] = {"MONO", "STEREO"};
  if ((sample_format < INT8) || (sample_format > FLOAT64) || (layout < MONO)
      || (layout > STEREO) || (sampling_rate < 0)) {
    return "{Invalid}";
  }
  return util::StrCat("{", format_strings[sample_format], ", ",
      layout_strings[layout], ", ", sampling_rate, "}");
}

uint32_t GetSampleFormatBytes(SampleFormat sample_format) {
  switch (sample_format) {
    case SampleFormat::INT8:
      return 1;
    case SampleFormat::INT16:
      return 2;
    case SampleFormat::INT32:
      return 4;
    case SampleFormat::INT64:
      return 8;
    case SampleFormat::FLOAT32:
      return 4;
    case SampleFormat::FLOAT64:
      return 8;
    default:
      ASSERT(false);
  }
}

int GetChannelLayoutChannels(ChannelLayout channel_layout) {
  switch (channel_layout) {
  case ChannelLayout::MONO:
    return 1;
  case ChannelLayout::STEREO:
    return 2;
  default:
    ASSERT(false);
  }
}


} // namespace audio