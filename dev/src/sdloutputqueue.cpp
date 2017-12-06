#include "sdloutputqueue.h"

#include "status_macros.h"
#include "strcat.h"

using ::util::Status;
using ::util::StatusOr;
using ::util::StrCat;

namespace audio {
namespace {

constexpr int kAudioBufferSize = 4096;

StatusOr<SDL_AudioSpec> FormatToAudioSpec(Format format) {
  SDL_AudioSpec audio_spec;
  SDL_zero(audio_spec);

  audio_spec.channels = GetChannelLayoutChannels(format.layout);

  switch (format.sample_format) {
  case SampleFormat::INT8:
    audio_spec.format = AUDIO_S8;
    break;
  case SampleFormat::INT16:
    audio_spec.format = AUDIO_S16;
    break;
  case SampleFormat::INT32:
    audio_spec.format = AUDIO_S32;
    break;
  case SampleFormat::INT64:
    return util::InvalidArgumentError("Invalid SDL sample format INT64");
  case SampleFormat::FLOAT32:
    audio_spec.format = AUDIO_F32;
  case SampleFormat::FLOAT64:
    return util::InvalidArgumentError("Invalid SDL sample format FLOAT64");
  default:
    ASSERT(false);
  }

  audio_spec.freq = format.sampling_rate;
  audio_spec.samples = kAudioBufferSize;

  return audio_spec;
}

} // namespace

SDLOutputQueue::SDLOutputQueue(SDL_AudioDeviceID device_id, Format format, 
    bool device_owned) : OutputQueue(format), device_id_(device_id), 
    device_owned_(device_owned) {}

SDLOutputQueue::~SDLOutputQueue() {
  if (device_owned_) SDL_CloseAudioDevice(device_id_);
}

int64_t SDLOutputQueue::GetQueuedSamplesSize() const {
  return SDL_GetQueuedAudioSize(device_id_) / 
      GetSampleFormatBytes(format_.sample_format);
}

Status SDLOutputQueue::QueueBytes(void* data, int64_t data_size) {
  if (SDL_QueueAudio(device_id_, data, data_size) != 0) {
    return util::IOError(
        StrCat("Failed to queue audio with message: '", SDL_GetError(), "'"));
  }
  return util::OkStatus;
}

util::StatusOr<std::unique_ptr<SDLOutputQueue>> Create(Format format) {
  ASSIGN_OR_RETURN(auto spec, FormatToAudioSpec(format));
  SDL_AudioSpec unused;
  SDL_AudioDeviceID device_id =
      SDL_OpenAudioDevice(NULL, 0, &spec, &unused, 0);
  if (device_id <= 0) {
    return util::IOError(
        StrCat("Failed to open audio device with message: '", SDL_GetError(), 
        "'"));
  }
  return std::make_unique<SDLOutputQueue>(device_id, format, true);
}

StatusOr<std::unique_ptr<SDLOutputQueue>> SDLOutputQueue::Create(
    SDL_AudioDeviceID device_id, Format format) {
  if (device_id <= 0) {
    return util::InvalidArgumentError("Invalid device_id == 0.");
  }
  return std::make_unique<SDLOutputQueue>(device_id, format, false);
}


} // namespace audio