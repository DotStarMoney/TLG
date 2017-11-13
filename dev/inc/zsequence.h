#ifndef AUDIO_ZSEQUENCE_H_
#define AUDIO_ZSEQUENCE_H_

#include <atomic>
#include <functional>
#include <memory>
#include <stdint.h>

#include "statusor.h"

namespace audio {

// ZSequence 
//
// An immutable interface for accessing ZSEQ sequences, see:
// https://docs.google.com/document/d/1kxSpkjpbimH58kEPS9KCm4EAqHIkJn9A353WsWbMi9c/edit?usp=sharing
//
// Accessing a ZSequence is very lightweight, and very little "upfront" 
// parsing or loading is done. The ZSEQ binary data is read directly when 
// interfacing with a ZSequence.
//
// ZSequences can be read through the constant accessors, or by creating a 
// playlist associated with the time varying data. A playlist is a read once
// object that takes a structure of callbacks, and issues one of these 
// callbacks corresponding to an event each time its Advance method is called.
//
// Herin, we will often reference "ticks," which form a duration when paired
// with a "tempo," itself in ticks/second. The tempo can change per a master
// event, therefore the interpretation of an amount of "ticks" depends on the
// master event sequence.
//
// All playlist events of a ZSequence are said to occur in parallel, that is to
// say: ALL events belong to the same timeline.
//
// The interpretation of parameters and combined parameters like channel pan 
// mixing with master pan are left up to the user.
//
class ZSequence {
  friend class Playlist;
 public:
  typedef const uint8_t* Stream;

 private:

  // The base class for all playlist types.
  class Playlist {
   public:
    // Process the next event in this playlist, returning true if playback of
    // this playlist has ended.
    //
    // If a playlist has ended, further calls to Advance will return an error.
    util::StatusOr<bool> Advance();
    ~Playlist();
   protected:
    // Process the given pattern event in this playlist. Since playlist events
    // are shared between all playlists, we only specialize pattern
    // advancement. AdvancePattern will not be called when cursor_ points to
    // non-specialized pattern events (namely delay and return).
    //
    // Returns true if no more events need to be read, false otherwise.
    virtual util::StatusOr<bool> AdvancePatternEvent() = 0;

    // Callbacks shared by all playlists
    struct Callbacks {
      // Delay the next event by a duration in ticks.
      std::function<void(uint16_t duration)> rest_callback;
    };
    explicit Playlist(ZSequence* parent, Stream playlist, Callbacks callbacks);
   private:
    // Get the Stream of a playlist pattern.
    Stream GetPatternData(int pattern);
    // Process the next pattern event: unlike AdvancePatternEvent, this method
    // handles all event types, including those shared between all pattern
    // types (namely delay and return). Takes the number of read codes thusfar
    // to prevent infinite sequence reading loops.
    //
    // Return true if playback of the pattern has ended, false otherwise.
    util::StatusOr<bool> AdvanceAnyPatternEvent(int* read_code_count);
    
    // **These sections are split the way they are so that we can utilize a
    // specific initialization order**

   private:
    // True if playback is within a pattern. This is a class member as we use
    // it to designate on first advancement that we are not yet in a pattern
    // (all subsequent calls to Advance start with the cursor in a pattern or
    // with playback ended)
    bool in_pattern_;
    // When true, take the coda event and toggle coda_. Initializes to false.
    bool coda_;
    // Used to track the number of repeats left when observing a repeat event.
    // A special value (255) is used to track when there is no repeat in 
    // progress.
    uint8_t repeat_counter_;
    // When pattern playback has finished, the location in the playlist at
    // which the cursor resumes playback.
    Stream return_to_;

    // The base of the playlist data as originally passed into the constructor.
    Stream const playlist_base_;
   protected:
    // Callbacks shared by all playlist types.
    Callbacks shared_callbacks_;

    // The current byte position in the playlist.
    Stream cursor_;
   private:
    // A pointer to the table of 16bit pattern offsets.
    Stream const pattern_start_table_;
    // The parent sequence, we track it here for access to the playlist
    // reference counter.
    ZSequence* parent_;

    // Flags that this playlist is out of events to play, and attempting to
    // advance it any further will result in an error.
    bool completed_;
  };

 public:

  // A note event playlist for triggering callbacks pertaining to articulation
  // events. This class is the interface to channel note data.
  //
  // Articulation parameters are initialized:
  //   Note Range = 0
  //   Velocity = 100%
  //   Hold Duration = 0
  //   Total Duration = 0
  //
  class NoteEventPlaylist : public Playlist {
   public:
    friend class ZSequence;
    struct Callbacks : public Playlist::Callbacks {
      // Articulate a note at the given pitch and velocity for hold_duration
      // ticks. Delay the next event by total_duration ticks (this is from the
      // start of the articulation, not in addition to the hold_duration).
      std::function<void(int16_t pitch_offset, double velocity,
          uint16_t hold_duration, 
          uint16_t total_duration)> articulate_callback;
    };
   private:
    NoteEventPlaylist(ZSequence* parent, Stream playlist, Callbacks callbacks);
    util::StatusOr<bool> AdvancePatternEvent() override;

    // Callbacks for the Advance method.
    Callbacks callbacks_;
    // An offset added to read note codes used to calculate actual note values.
    int16_t note_range_;
    // The current velocity.
    double velocity_;
    // The current number of ticks to hold articulated notes.
    uint16_t hold_duration_;
    // The current number of ticks over which an articulated note is playing,
    // but not neccessarily held. This means a note event can represent an
    // articulation event followed by an optional rest.
    uint16_t total_duration_;
  };

  // A parameter event playlist for triggering callbacks pertaining to 
  // channel parameter events. This class is the interface to channel
  // parameter data.
  //
  // Pitch shift is initialized to 0 for the purposes of pitch adjust events.
  //
  class ParameterEventPlaylist : public Playlist {
   public:
    friend class ZSequence;
    struct Callbacks : public Playlist::Callbacks {
      // Set the channel volume and delay the next event by a number of ticks.
      //
      // Volume ranges from [0, 1], with 0.5 representing no volume change.
      std::function<void(double volume, uint16_t total_duration)>
          set_volume_callback;
      // Set the channel pan and delay the next event by a number of ticks.
      //
      // Pan ranges from [-1, 1], for, respectively, a fully L and R pan.
      std::function<void(double pan, uint16_t total_duration)>
          set_pan_callback;
      // Set the channel pitch shift in semitones and delay the next event by a
      // number of ticks.
      std::function<void(double pitch_shift, uint16_t total_duration)>
          set_pitch_shift_callback;
      // Set the channel vibrato range in semitones and delay the next event by
      // a number of ticks.
      std::function<void(double vibrato_range, uint16_t total_duration)>
          set_vibrato_range_callback;
      // Set the channel playback instrument and delay the next event by a
      // number of ticks.
      std::function<void(uint8_t instrument, uint16_t total_duration)>
          set_instrument_callback;
    };
   private:
    ParameterEventPlaylist(ZSequence* parent, Stream playlist,
        Callbacks callbacks);
    util::StatusOr<bool> AdvancePatternEvent() override;
     
    Callbacks callbacks_;
    // The current pitch shift (in 64ths of a semitone). This is tracked 
    // because pitch adjust event codes must know a current "value" of the 
    // pitch shift should they adjust it.
    int16_t pitch_shift_64th_;
  };

  // A parameter event playlist for triggering callbacks pertaining to master
  // parameter events. This class is the interface to master parameter data.
  class MasterEventPlaylist : public Playlist {
   public:
    friend class ZSequence;
    struct Callbacks : public Playlist::Callbacks {
      // Set the channel volume and delay the next event by a number of ticks.
      //
      // Volume ranges from [0, 1], with 0.5 representing no volume change.
      std::function<void(double volume, uint16_t total_duration)>
          set_master_volume_callback;
      // Set the master pan and delay the next event by a number of ticks.
      //
      // Pan ranges from [-1, 1], for, respectively, a fully L and R pan.
      std::function<void(double pan, uint16_t total_duration)>
          set_master_pan_callback;
      // Set the master pitch shift in semitones and delay the next event by a
      // number of ticks.
      std::function<void(double pitch_shift, uint16_t total_duration)>
          set_master_pitch_shift_callback;
      // Set the tempo in ticks/second, and delay the next event by a number of
      // ticks.
      std::function<void(uint8_t tempo, uint16_t total_duration)>
          set_tempo_callback;
    };
   private:
    MasterEventPlaylist(ZSequence* parent, Stream playlist, 
        Callbacks callbacks);
    util::StatusOr<bool> AdvancePatternEvent() override;

    Callbacks callbacks_;
  };

  util::StatusOr<std::unique_ptr<ZSequence>> Create(
      std::unique_ptr<const uint8_t> zseq);

  // Get event playlists for a specific channel.
  std::unique_ptr<NoteEventPlaylist> CreateNoteEventPlaylist(int channel,
      NoteEventPlaylist::Callbacks callbacks);
  std::unique_ptr<ParameterEventPlaylist> CreateParameterEventPlaylist(
      int channel, ParameterEventPlaylist::Callbacks callbacks);

  // Get a master event playlist.
  std::unique_ptr<MasterEventPlaylist> CreateMasterEventPlaylist(
      MasterEventPlaylist::Callbacks callbacks);

  // The instrument bank reference by the instruments channels.
  uint8_t bank() const { return bank_; }
  // The number of channels in this sequence.
  int channels() const { return channels_; }
  // The starting instrument for a channel. We don't inline this becase getting
  // the starting instrument is **slightly** more complicated as we don't save
  // the values explicitly.
  uint8_t start_instrument(int channel) const;
  // The playback priority number for a channel. Playback priority indicates
  // the order channels should be overidden should a sound need to play using
  // the playback resources used by a zsequence channel.
  //
  // The priority number is the lexicographic permutation number of the channel
  // numbers 1 through 8, with a channel number earlier in the sequence being
  // at a higher priority.
  uint16_t channel_priority() const { return channel_priority_; }
  // 8 2bit values specifying the locations in an effect pipeline to which a
  // channels should route their samples.
  uint16_t channel_routing() const { return channel_routing_; }
  // The initial tempo for this sequence in ticks/second.
  uint8_t start_tempo() const { return start_tempo_; }
 private:  
  // Mosty responsible for initializing const members...
  ZSequence(uint8_t bank, int channels, uint16_t channel_priority, 
      uint16_t channel_routing, uint8_t start_tempo, Stream master_playlist,
      const uint16_t* channel_playlist_table);

  // Get a Stream to the start of an individual channel's data
  Stream GetChannelDataStart(int channel) const;

  // A reference counter the tracks the number of playlists created by this
  // ZSequence. This is used so that ZSequence can safely abort if there are
  // outstanding references to playlists when a ZSequence destructs.
  std::atomic_uint32_t playlist_refs_;
  
  // See docs for accessors.
  const uint8_t bank_;
  const int channels_;
  const uint16_t channel_priority_;
  const uint16_t channel_routing_;
  const uint8_t start_tempo_;

  // The master playlist bytes.
  const Stream master_playlist_;
  // The channel playlist table bytes.
  const uint16_t* channel_playlist_table_;

  // The ZSEQ binary data.
  std::unique_ptr<const uint8_t> sequence_;
};

} // namespace audio

#endif // AUDIO_ZSEQUENCE_H_