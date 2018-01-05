#ifndef AUDIO_AUDIOCONTEXT_H_
#define AUDIO_AUDIOCONTEXT_H_

#include <array>
#include <variant>
#include <vector>

#include "instrument.h"
#include "resourcemanager.h"
#include "sampledatam16.h"
#include "status.h"
#include "statusor.h"
#include "sharedsamplers16.h"
#include "audiosystem.h"

namespace audio {

// THE FUQ is a "context configuration step?" (same as barrier resolution?)

// is thread safe

// arm resources in returned "sound"

// explain priorities

class AudioContext {
 public: 
  static constexpr int kSamplerChannels = 8;
  AudioContext();

  // Create a sound for playback. Specific playback controls are exposed in the
  // returned Sound. If the Sound goes out of scope, playback will cease.
  //
  // Takes the channel in which the created sound will play at the provided 
  // priority
  class Sound;
  util::StatusOr<std::unique_ptr<Sound>> CreateSound(uint8_t channel, 
      int32_t priority = 0);

  // Create a sequence for playback. Specific playback controls are exposed in
  // the returned Sequence. If the Sequence goes out of scope, playback will
  // cease.
  //
  // Takes the channels over which the created sequence's channels will play 
  // at the provided priorities. The number of elements in the list of
  // channels and priorities must match the number of playback channels in the
  // provided sequence resource.
  class Sequence;
  util::StatusOr<std::unique_ptr<Sequence>> CreateSequence(
      ResourceManager::MapID res, const std::vector<uint8_t>& channels,
      const std::vector<int32_t>& priorities);
  // Overload for using uniform values of priority.
  util::StatusOr<std::unique_ptr<Sequence>> CreateSequence(
      ResourceManager::MapID res, const std::vector<uint8_t>& channels,
      int32_t priority = 0);
  // Overloads for string resource IDs
  util::StatusOr<std::unique_ptr<Sequence>> CreateSequence(
      std::string_view res, const std::vector<uint8_t>& channels,
      const std::vector<int32_t>& priorities);
  util::StatusOr<std::unique_ptr<Sequence>> CreateSequence(
      std::string_view res, const std::vector<uint8_t>& channels,
      int32_t priority = 0);

  // An object used for single channel audio playback of a SampleDataM16 or 
  // Instrument resource.
  // 
  // The parameters default to the following:
  //
  //   pitch = 0 (no pitch shift)
  //   volume = 1 (no change)
  //   pan = 0 (pan to the middle)
  //   vibrato_range = 0 (no vibrato)
  //
  class Sound {
    friend class AudioContext;
   public:
    // Will return the resources we hold, and ask the SharedSamplerS16 we're
    // currently referencing to maybe disarm itself.
    ~Sound();

    // Rewind and start playin' the provided sound.
    //
    // This pitch is different from the normal pitch parameter, in that if this
    // sound plays an Instrument, the instrument split is chosen from this
    // pitch.
    //
    // The resource played back can be either a SampleDataM16 or an Instrument.
    void Play(ResourceManager::MapID res, float pitch = 0);
    void Play(std::string_view res, float pitch = 0);

    // Stop playback of the sound.
    void Stop();
    
    void set_pitch(float pitch);
    float pitch() const;

    void set_volume(float volume);
    float volume() const;

    void set_pan(float volume);
    float pan() const;

    void set_vibrato_range(float vibrato_range);
    float vibrato_range() const;
   private:
    Sound(AudioContext* parent);

    // The context that created this Sound.
    AudioContext* parent_;
    // True if this Sound is armed with an instrument.
    bool has_instument() const;

    // A token received from a SharedSamplerS16::Play. During desturction we
    // pass this to SharedSamplerS16::Disarm so that it may cease playback, and
    // more importantly, drop (disarm) any references to our sample/instrument
    // data.
    SharedSamplerS16::Token token_;

    // The current set of playback parameters. 
    //
    // Copied out by SharedSamplerS16 during barrier resolution.
    SamplerS16::Parameters playback_params_;
    // The sample we are armed with for playback.
    //
    // Copied out by SharedSamplerS16 during barrier resolution.
    ResourceManager::ResourcePtr<SampleDataM16> sample_;
    // An optional instrument we are armed with for playback, used to determine
    // the playback sample.
    //
    // The pointers to the characteristics are copied out by SharedSamplerS16
    // during barrier resolution.
    ResourceManager::ResourcePtr<Instrument> instrument_;
  };





  // An object used for playback of a ZSequence resource.
  //
  // Since a sequence will set master parameters on its own, to set any global
  // sequence parameters you must first call the respective override method,
  // at which point the master sequence parameters will lock to the value of
  // the override parameter. Override parameters default to:
  //
  //   pitch = 0 (no pitch shift)
  //   volume = 1 (no change)
  //   pan = 0 (pan to the middle)
  //
  class Sequence {
    friend class AudioContext;
    friend class SharedSamplerS16;
   public:
    // Rewind and start playback of the sequence.
    void Play(ResourceManager::MapID res);
    void Play(std::string_view res);

    // Pause playback.
    void Pause();
    // Stop playback.
    void Stop();

    void EnablePitchOverride(bool _override = true);
    void SetPitchOverride(float pitch);
    float pitch_override() const;

    void EnableVolumeOverride(bool _override = true);
    void SetVolumeOverride(float volume);
    float volume_override() const;

    void EnablePanOverride(bool _override = true);
    void SetPanOverride(float pan);
    float pan_override() const;
   private:
    Sequence();
  };

 private:
   std::array<SharedSamplerS16, kSamplerChannels> samplers_;
  
};

} // namespace audio

#endif // AUDIO_AUDIOCONTEXT_H_