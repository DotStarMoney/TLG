#include "audio_format.h"
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

} // namespace audio