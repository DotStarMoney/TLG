#ifndef AUDIO_ZSEQUENCE_H_
#define AUDIO_ZSEQUENCE_H_

#include <atomic>
#include <functional>
#include <memory>
#include <stdint.h>

#include "statusor.h"

namespace audio {


// TODO(?): DOC ME, wtf are ticks?!
class ZSequence {
  friend class Playlist;
 public:
  typedef const uint8_t* Stream;

 private:

  // The base class for all playlist types.
  class Playlist {
   public:
    // Process the next event in this playlist, returning false if playback of
    // this playlist has ended.
    util::StatusOr<bool> Advance();
    ~Playlist();
   protected:
    // Process the next pattern event in this playlist. Since playlist events
    // are shared between all playlists, we only specialize pattern
    // advancement. AdvancePattern will not be called when cursor_ points to
    // non-specialized pattern events (namely delay and return).
    virtual util::Status AdvancePatternEvent() = 0;

    // Callbacks shared by all playlists
    struct Callbacks {
      std::function<void(uint16_t duration)> rest_callback;
    };
    explicit Playlist(ZSequence* parent, Stream playlist);
   private:
    // Get the Stream of a playlist pattern.
    Stream GetPatternData(int pattern);
    // Process the next pattern event: unlike AdvancePatternEvent, this method
    // handles all event types, including those shared between all pattern
    // types (namely delay and return)
    util::StatusOr<bool> AdvanceAnyPatternEvent();
    
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
    // The current byte position in the playlist.
    Stream cursor_;
   private:
    // A pointer to the table of 16bit pattern offsets.
    Stream const pattern_start_table_;
    // The parent sequence, we track it here for access to the playlist
    // reference counter.
    ZSequence* parent_;
  };

 public:

  // A note event playlist that supports a singular Advance method for
  // triggering callbacks pertaining to articulation events. This class is the
  // interface to channel note data.
  //
  // Articulation parameters are initialized:
  //   Note Range = 0
  //   Velocity = 100%
  //   Hold Duration = 0
  //   Total Duration = 0
  //
  class NoteEventPlaylist : public Playlist {
   public:
    struct Callbacks : public Playlist::Callbacks {
      std::function<void(int16_t pitch_offset, double velocity,
          uint16_t hold_duration, 
          uint16_t duration)> articulate_callback;
    };
   private:
    NoteEventPlaylist(ZSequence* parent, Stream playlist, Callbacks callbacks);
    util::Status AdvancePatternEvent() override;

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




  // Get a note event playlist for this channel.
  NoteEventPlaylist CreateNoteEventPlaylist(int channel, 
      NoteEventPlaylist::Callbacks callbacks) const;

  // The number of channels in this sequence.
  int channels() const;
 private:
  
  // A reference counter the tracks the number of playlists created by this
  // ZSequence. This is used so that ZSequence can safely abort if there are
  // outstanding references to playlists when ZSequences destructs.
  std::atomic_uint32_t playlist_refs_;

  // The ZSEQ binary data.
  std::unique_ptr<const uint8_t> sequence_;
};

} // namespace audio

#endif // AUDIO_ZSEQUENCE_H_