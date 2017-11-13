#include "sdl_audio_utils.h"

#include "brr_file.h"
#include "make_cleanup.h"
#include "SDL.h"
#include "strcat.h"
#include "audio_format.h"

using ::util::Status;
using ::std::string_view;

namespace sdl {
namespace util {

Status WavToBRR(string_view source_file, string_view dest_file,
    bool opt_for_resynth) {
  
  uint32_t sample_data_size;
  int16_t* sample_data;
  SDL_AudioSpec wav_spec;

  if (SDL_LoadWAV(source_file.data(), &wav_spec, 
      reinterpret_cast<uint8_t**>(&sample_data), &sample_data_size) == NULL) {
    return ::util::IOError(::util::StrCat("SDL failed to load Wav with: (", 
        SDL_GetError(), ")"));
  }

  ::util::MakeCleanup free_wav([sample_data](){ 
        SDL_FreeWAV(reinterpret_cast<uint8_t*>(sample_data)); 
      });

  // BRR files are exclusively for signed 16b mono audio data, so we have to
  // bail if its something else.
  if ((wav_spec.format != AUDIO_S16) || (wav_spec.channels != 1)) {
    return ::util::FormatMismatchError(::util::StrCat("Expected 16b signed "
        "mono sample data, instead got channels:", wav_spec.channels,
        " SDL_AudioFormat:", wav_spec.format));
  }

  // sample_data_size is divided by two since its in bytes, and we need words.
  std::vector<int16_t> sample_data_v(sample_data, 
      sample_data + sample_data_size / 2);

  return audio::SaveBRR(dest_file, 
      {sample_data_v, static_cast<audio::SampleRate>(wav_spec.freq), 
          opt_for_resynth});
}

} // namespace util
} // namespace sdl