#include "zsequence.h"

#include "assert.h"
#include "audio_defaults.h"
#include "bit_utils.h"
#include "status.h"
#include "status_macros.h"

using ::util::Status;
using ::util::StatusOr;
using ::util::StrCat;

// TODO(?): Add reference counting to sampledatam16

//
// Herein "streams" refer to ZSequence byte data
//
namespace audio {
namespace {

constexpr int kStopPatternEventCode = 0xff;

// Return a byte from, and advance to the next value, a stream.
uint8_t GetUByteAndInc(ZSequence::Stream* stream) {
  return *(++(*stream));
}

// Takes the current stream byte, and takes its division by 255 as a percentage
// of the passed in value. Returns this, and advances the stream.
uint16_t TakeUBytePercentageAndInc(uint16_t value, ZSequence::Stream* stream) {
  return static_cast<uint16_t>(value * 
      (static_cast<double>(GetUByteAndInc(stream)) / 255.0));
}

// An articulation read/updated from a note pattern data stream.
struct ArticulationData {
  uint8_t note_code;
  uint16_t hold_duration;
  uint16_t total_duration;
  uint8_t velocity;
};

// Parse the note articulation pattern event, advancing the passed in event 
// stream to the next event and populating the note data to reflect the 
// values in the articulation event.
Status ReadArticulationEvent(ZSequence::Stream* stream,
    ArticulationData* art_data) {

  const uint8_t event_code = GetUByteAndInc(stream);

  // Get the note code from the low 5 bits: 0 - 31
  art_data->note_code = event_code & 0x1f;
  // Get the articulation type from the high 3 bits: 0 - 7, though there are
  // only 6 modes.
  const uint8_t art_type = (event_code & 0xe0) >> 5;

  if (art_type > 5) {
    return util::FormatMismatchError(
        StrCat("Articulation type out of range: (", art_type, " > 5)"));
  }
  switch (art_type) {
  // Mode 0: All articulation parameters change
  case 0:
    art_data->velocity = GetUByteAndInc(stream);
    art_data->total_duration = util::varint::GetVarintAndInc(stream);
    art_data->hold_duration = 
        TakeUBytePercentageAndInc(art_data->total_duration, stream);
    return util::OkStatus;
  // Mode 1: Change duration and velocity, set hold to 100%
  case 1:
    art_data->velocity = GetUByteAndInc(stream);
    art_data->total_duration = util::varint::GetVarintAndInc(stream);
    art_data->hold_duration = art_data->hold_duration;
    return util::OkStatus;
  // Mode 2: Only velocity changes.
  case 2:
    art_data->velocity = GetUByteAndInc(stream);
    return util::OkStatus;
  // Mode 3: Change duration and hold, but leave velocity.
  case 3:
    art_data->total_duration = util::varint::GetVarintAndInc(stream);
    art_data->hold_duration =
       TakeUBytePercentageAndInc(art_data->total_duration, stream);
    return util::OkStatus;
  // Mode 4: Change duration, set hold to 100%, leave velocity
  case 4:
    art_data->total_duration = util::varint::GetVarintAndInc(stream);
    art_data->hold_duration = art_data->hold_duration;
    return util::OkStatus;
  // Mode 5: Nothing changes
  case 5:
    return util::OkStatus;
  default:
    ASSERT(false);
  }
}
// Given a stream pointing to a varint, returns the address of the stream
// pointer plus the value of the varint and advances the stream to the next
// value.
const uint8_t* GetVarintAsOffsetAndInc(ZSequence::Stream* stream) {
  ZSequence::Stream stream_base = *stream;
  return stream_base + util::varint::GetVarintAndInc(stream);
}

// Marks Playlist::repeat_ as uninitialized, or, "not currently repeating"
constexpr uint8_t kRepeatUninitialized = 255;
// We wrap this enum in a namespace to avoid poluting the translation unit
// namespace.
namespace PlaylistEvents {
enum PlaylistEventsE : uint8_t {
  kJump = 0xb1,
  kCoda = 0xb2,
  kRepeat = 0xb0,
  kStop = 0xff
};
} // namespace PlaylistEvents

} // namespace

// ***************************************************************************
// *                                Playlist                                 *
// ***************************************************************************

// Playlist will point to the next value after being processed by
// GetVarintAsOffsetAndInc, which in the case of a playlist is the pattern
// table.
//
// Increases the parent playlist reference counter.
ZSequence::Playlist::Playlist(ZSequence* parent, Stream playlist) : 
    in_pattern_(false),
    coda_(false),
    repeat_counter_(kRepeatUninitialized),
    return_to_(0),
    playlist_base_(playlist),
    cursor_(GetVarintAsOffsetAndInc(&playlist)),
    pattern_start_table_(playlist),
    parent_(parent) {
  ++(parent_->playlist_refs_);
}

// Decrease the parent playlist reference counter.
ZSequence::Playlist::~Playlist() { --(parent_->playlist_refs_); }

ZSequence::Stream ZSequence::Playlist::GetPatternData(int pattern) {
  const uint16_t offset = 
      *(reinterpret_cast<const uint16_t*>(pattern_start_table_) + pattern);
  return playlist_base_ + offset;
}

StatusOr<bool> ZSequence::Playlist::AdvanceAnyPatternEvent() {
  
}

StatusOr<bool> ZSequence::Playlist::Advance() {
  for(;;) {
    if (in_pattern_) {
      ASSIGN_OR_RETURN(bool pattern_complete, AdvanceAnyPatternEvent());
      if (!pattern_complete) return true; 
      // Return the cursor to the next playlist event
      ASSERT_NE(return_to_, 0);
      cursor_ = return_to_;

      in_pattern_ = false;
    }

    // Check if the high bit is clear, and therefore, if we are referencing a
    // pattern.
    if ((*cursor_ & 0x80) == 0) {
      // Set the cursor return.
      return_to_ = cursor_ + 1;
      // Set the cursor based on the pattern start table
      cursor_ = GetPatternData(*cursor_);
      in_pattern_ = true;
      continue;
    }

    const uint8_t playlist_event = GetUByteAndInc(&cursor_);
    switch (playlist_event) {
      case PlaylistEvents::kJump: {
        const uint16_t offset = util::varint::GetVarintAndInc(&cursor_);
        cursor_ = playlist_base_ + offset;
        break;
      }
      case PlaylistEvents::kCoda: {
        const uint16_t offset = util::varint::GetVarintAndInc(&cursor_);
        const bool original_coda = coda_;
        coda_ = !coda_;
        if (original_coda) cursor_ = playlist_base_ + offset;
        break;
      }
      case PlaylistEvents::kRepeat: {
        const uint8_t repeats = GetUByteAndInc(&cursor_);
        if (repeat_counter_ == kRepeatUninitialized) {
          repeat_counter_ = repeats - 1;
          // Rewind the cursor to the pattern immediately preceeding this
          // repeat event.
          cursor_ -= 3;
          if ((*cursor_ & 0x80) != 0) {
            return util::FormatMismatchError(StrCat("Repeat, but previous "
                "playlist event is not a pattern. (", 
                static_cast<int>(*cursor_), ")"));
          }
        } else if (repeat_counter_ > 0) {
          --repeat_counter_;
          cursor_ -= 3;
        } else {
          repeat_counter_ = kRepeatUninitialized;
        }
        break;
      }
      default: 
        return util::FormatMismatchError(StrCat("Unrecognized playlist event. "
            "(", playlist_event, ")"));
    }
  }
}

// ***************************************************************************
// *                            NoteEventPlaylist                            *
// ***************************************************************************

ZSequence::NoteEventPlaylist::NoteEventPlaylist(ZSequence* parent,
    Stream playlist, Callbacks callbacks) : Playlist(parent, playlist),
    callbacks_(callbacks), note_range_(0), velocity_(kVolume100P),
    hold_duration_(0), total_duration_(0) {}

StatusOr<bool> ZSequence::NoteEventPlaylist::AdvancePattern() {
  
}

} // namespace audio