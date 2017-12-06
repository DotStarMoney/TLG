#ifndef AUDIO_SDLOUTPUTQUEUE_H_
#define AUDIO_SDLOUTPUTQUEUE_H_

#include <memory>

#include "SDL.h"
#include "status.h"
#include "statusor.h"
#include "outputqueue.h"

namespace audio {

// An audio queue interfacing with SDL. Thread safe.
class SDLOutputQueue : public OutputQueue {
 public:
  ~SDLOutputQueue();
  
  int64_t GetQueuedSamplesSize() const override;

  // This will open an audio device iff such a device can play audio with the 
  // desired format. Note WHAT audio device is opened isn't specified, as long
  // as it meets the criteria in the provided format.
  static util::StatusOr<std::unique_ptr<SDLOutputQueue>> Create(Format format);
  // Create an SDLAudioQueue from a format matching the audio spec obtained 
  // from SDL_OpenAudioDevice and the device id from the same call. We do not
  // own this device.
  static util::StatusOr<std::unique_ptr<SDLOutputQueue>> Create(
      SDL_AudioDeviceID device_id, Format format);

protected:

  util::Status QueueBytes(void* data, int64_t data_size) override;

 private:
  SDLOutputQueue(SDL_AudioDeviceID device_id, Format format,
      bool device_owned);

  const SDL_AudioDeviceID device_id_;
  // Tracks whether the device_id is one from a call to 
  // SDL_OpenAudioDevice that we manager, or an external one.
  const bool device_owned_;
};
  
} // namespace audio

#endif // AUDIO_SDLOUTPUTQUEUE_H_