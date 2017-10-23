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

  const int total_samples = 1e6;
  const int total_bytes = total_samples * 2;
  const int total_stereo_samples = total_samples * 2;

  std::cout << "Generating " << total_samples << " samples." << std::endl;
  std::vector<int16_t> sample_data(total_stereo_samples);
  sampler->ProvideNextSamples(sample_data.begin(), total_samples, 0);

  std::cout << "Queueing entire buffer." << std::endl;
  int success = SDL_QueueAudio(dev, sample_data.data(), total_bytes);

  std::cout << "Waiting for 5s." << std::endl;
  SDL_Delay(10000);

  //
  SDL_CloseAudioDevice(dev);
  return success;
}

int main(int argc, char** argv) {

  std::cout << "Reading BRR." << std::endl;
  auto sample = audio::LoadBRR("res/vlaclose16m32000.brr").ConsumeValueOrDie();

  audio::AudioSystem audio_system(audio::_32K);
  audio::StereoSampler16 sampler(&audio_system);

  sampler.Arm(sample.get());

  sampler.Play();

  return PlaybackTest(&sampler, *sample.get());

  // TODO(?): Move "too high" check of sampler into IterateNextSample
  // TODO(?): Test looping
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