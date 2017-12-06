#include <iostream>
#include <chrono>

#include "assert.h"
#include "audiosystem.h"
#include "audiocontext.h"
#include "resourcemanager.h"
#include "instrument.h"
#include "sampledatam16.h"
#include "SDL.h"


int main(int argc, char** argv) {
/*
  ResourceManager resource_manager;
  resource_manager.MapIDToURI("IORGAN",  "ins/organ.ins");
  resource_manager.MapIDToURI("SPRIZE", "snd/prize.brr");

  resource_manager.RegisterDeserializer("ins", 
      audio::Instrument::Deserialize);
  resource_manager.RegisterDeserializer("brr",
      audio::SampleDataM16::Deserialize);
  
  audio::AudioSystem audio_system(audio::SampleRate::_32K);
  // delay, filter (can be combined with delay), compressor, out
  // audio_system.SetTap(0, 1) or something

  audio::AudioContext main_context;

  audio_system.SetContext(&main_context);


  auto sound = main_context.CreateSound(0).ConsumeValueOrDie();

  ASSERT_EQ(resource_manager.Load("IORGAN"), util::OkStatus); 
  ASSERT_EQ(resource_manager.Load("SPRIZE"), util::OkStatus);

  for (;;) {
    
    if ((rand() % 1000) == 0) {
      sound->Play(
          ((rand() % 2) == 0) ? "IORGAN" : "SPRIZE", 
          (rand() % 17) - 8);
    }
  
    audio_system.Sync();

    SDL_Delay(20);
  }
  */
  
  // TODO(?): Build Clang Fix or whatever for Google style guide into workflow
  // TODO(?): Need to setup tests, need way to run tests, and to write tests
  //          for existing classes
  // TODO(?): Need logging capabilities and need to log in "asserts,"
  // TODO(?): Add stack trace to status
  // TODO(?): Determine what to do about debug flags
  // TODO(?): Look into allocator, worth tracking memory usage?
  // TODO(?): One final check to apply new stuff to existing code
  // TODO(?): Start having fun :D
  return 0;
}
