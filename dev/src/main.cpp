#include <iostream>
#include <chrono>

#include "assert.h"
#include "sdl_audio_utils.h"
#include "brr_file.h"
#include "stereosampler16.h"
#include "audiosystem.h"
#include "SDL.h"

int PlaybackTest(audio::StereoSampler16* sampler,
    const audio::MonoSampleData16& sampler_data) {
  if (SDL_Init(SDL_INIT_AUDIO) < 0) return 1;

  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;

  SDL_zero(want);
  want.freq = 32000;
  want.format = AUDIO_S16;
  want.channels = 2;
  want.samples = 4096;

  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (dev == 0) return 1;
  SDL_PauseAudioDevice(dev, 0);
  //

  std::vector<int16_t> sample_data((64000 + 64000) * 2);

  std::cout << "Generating 64000 samples." << std::endl;
  sampler->ProvideNextSamples(sample_data.begin(), 64000, 0);

  std::cout << "Releasing." << std::endl;
  sampler->Release();

  std::cout << "Generating 64000 more samples." << std::endl;
  sampler->ProvideNextSamples(sample_data.begin() + 128000, 64000, 64000);

  std::cout << "Queueing entire buffer." << std::endl;
  if (SDL_QueueAudio(dev, sample_data.data(), sample_data.size() * 2) != 0) {
    return 1;
  }

  std::cout << "Waiting for 2.5s." << std::endl;
  SDL_Delay(2500);

  //
  SDL_CloseAudioDevice(dev);
  return 0;
}

int main(int argc, char** argv) {

  std::cout << "Encoding BRR file...";
  ASSERT_EQ(sdl::util::WavToBRR(
          "res/littlelead.wav",
          "res/littlelead.brr",
          3114,
          3955,
          0, 100, 128, 100), 
      util::OkStatus);

  std::cout << "Reading BRR." << std::endl;
  auto sample = audio::LoadBRR("res/littlelead.brr").ConsumeValueOrDie();

  audio::AudioSystem audio_system(audio::_32K);
  audio::StereoSampler16 sampler(&audio_system);

  sampler.Arm(sample.get());
 
  sampler.Play(0);
  return PlaybackTest(&sampler, *sample.get());

  // TODO(?): Create the sequence format, write a player.

  // TODO(?): Build Clang Fix or whatever for Google style guide into workflow
  // TODO(?): Need to setup tests, need way to run tests, and to write tests
  //          for existing classes
  // TODO(?): Need logging capabilities and need to log in "asserts,"
  // TODO(?): Add stack trace to status
  // TODO(?): Improve file handling (scoped closers, status support and such)
  // TODO(?): Determine what to do about debug flags
  // TODO(?): Look into allocator, worth tracking memory usage?
  // TODO(?): One final check to apply new stuff to existing code
  // TODO(?): Start having fun :D!
  
}