#include "zsequence.h"

#include "assert.h"
#include "audio_defaults.h"
#include "bit_utils.h"
#include "status.h"
#include "status_macros.h"

using ::util::Status;
using ::util::StatusOr;
using ::util::StrCat;

namespace audio {
namespace {

constexpr int kStopPatternEventCode = 0xff;

// Return a byte from, and advance to the next value, a stream.
uint8_t GetUByteAndInc(ZSequence::Stream* stream) {
  return *(++(*stream));
}

// Return a word from, and advance to the next value, a stream.
uint16_t GetUWordAndInc(ZSequence::Stream* stream) {
  const uint16_t value = *reinterpret_cast<const uint16_t*>(*stream);
  *stream += 2;
  return value;
}

// Takes the current stream byte, and takes its division by 255 as a percentage
// of the passed in value. Returns this, and advances the stream.
uint16_t TakeUBytePercentageAndInc(uint16_t value, ZSequence::Stream* stream) {
  return static_cast<uint16_t>(value * 
      (static_cast<double>(GetUByteAndInc(stream)) / 255.0));
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

// The limit on the number of non-callback-triggering events playlist's will
// read in one call to Advance. Loosely followed: at worst one extra event is
// observed past this limit.
constexpr int kMaxAdvanceEvents = 4;

// We wrap these enums in namespaces to avoid poluting the translation unit
// namespace.
namespace PlaylistEvents {
enum PlaylistEventsE : uint8_t {
  kJump = 0xb1,
  kCoda = 0xb2,
  kRepeat = 0xb0,
  kStop = 0xff
};
} // namespace PlaylistEvents

namespace PatternEvents {
enum PatternEventsE : uint8_t {
  kDelay = 0xf0,
  kReturn = 0xff
};
} // namespace PatternEvents

namespace NotePatternEvents {
enum NotePatternEventsE : uint8_t {
  kSetNoteRange = 0xe1
};
} // namespace NotePatternEvents

namespace ParameterPatternEvents {
enum ParameterPatternEventsE : uint8_t {
  kSetVolume = 0x41,
  kSetPan = 0x42,
  kSetPitch = 0x45,
  kAddPitch = 0x46,
  kSetVibratoRange = 0x54,
  kSetInstrument = 0x69
};
} // namespace ParameterPatternEvents

namespace MasterPatternEvents {
enum MasterPatternEventsE : uint8_t {
  kSetMasterVolume = 0x41,
  kSetMasterPan = 0x42,
  kSetMasterPitchShift = 0x45,
  kSetTempo = 0x21
};
} // namespace MasterPatternEvents

} // namespace

ZSequence::ZSequence(uint8_t bank, int channels, uint16_t channel_priority, 
    uint16_t channel_routing, uint8_t start_tempo, Stream master_playlist,
    const uint16_t* channel_playlist_table) : 
    playlist_refs_(0), 
    bank_(bank),
    channels_(channels), 
    channel_priority_(channel_priority), 
    channel_routing_(channel_routing),
    start_tempo_(start_tempo), 
    master_playlist_(master_playlist),
    channel_playlist_table_(channel_playlist_table) {}

StatusOr<std::unique_ptr<ZSequence>> ZSequence::Create(
    std::unique_ptr<const uint8_t> zseq) {
  const uint8_t* cursor = zseq.get();

  if (GetUByteAndInc(&cursor) != 0x5a) {
    return util::FormatMismatchError("Wrong ID byte.");
  }

  const uint8_t bank = GetUByteAndInc(&cursor);
  const uint8_t tempo = GetUByteAndInc(&cursor);
  const int channels = GetUByteAndInc(&cursor);
  if ((channels < 1) || (channels > 8)) {
    return util::FormatMismatchError(StrCat("Channel out of range [1, 8]:", 
        channels));
  }
  const uint16_t channel_priority = GetUWordAndInc(&cursor);
  const uint16_t channel_routing = GetUWordAndInc(&cursor);
  const Stream master_playlist = util::varint::GetVarintAndInc(&cursor) + 
      zseq.get();
  const uint16_t* channel_playlist_table =
      reinterpret_cast<const uint16_t*>(cursor);

  auto seq = new ZSequence(bank, channels, channel_priority, channel_routing, 
      tempo, master_playlist, channel_playlist_table);
  seq->sequence_ = std::move(zseq);
 
  return std::unique_ptr<ZSequence>(seq);
}

ZSequence::Stream ZSequence::GetChannelDataStart(int channel) const {
  return sequence_.get() + *(channel_playlist_table_ + channel);
}

uint8_t ZSequence::start_instrument(int channel) const {
  // The first byte in a block of channel data is the instrument number
  return *GetChannelDataStart(channel);
}

std::unique_ptr<ZSequence::NoteEventPlaylist>
    ZSequence::CreateNoteEventPlaylist(int channel,
        NoteEventPlaylist::Callbacks callbacks) {
  const Stream channel_data = GetChannelDataStart(channel);
  const Stream note_data = channel_data + 
      *reinterpret_cast<const uint16_t*>(channel_data + 1);

  // We do this instead of make_unique here and below for to member access
  // reasons
  auto playlist = new NoteEventPlaylist(this, note_data, callbacks);
  return std::unique_ptr<NoteEventPlaylist>(playlist);
}
std::unique_ptr<ZSequence::ParameterEventPlaylist>
    ZSequence::CreateParameterEventPlaylist(
        int channel, ParameterEventPlaylist::Callbacks callbacks) {
  const Stream parameter_data = GetChannelDataStart(channel) + 3;

  auto playlist = new ParameterEventPlaylist(this, parameter_data, callbacks);
  return std::unique_ptr<ParameterEventPlaylist>(playlist);
}
std::unique_ptr<ZSequence::MasterEventPlaylist>
    ZSequence::CreateMasterEventPlaylist(
        MasterEventPlaylist::Callbacks callbacks) {

  auto playlist = new MasterEventPlaylist(this, master_playlist_, callbacks);
  return std::unique_ptr<MasterEventPlaylist>(playlist);
}

// ***************************************************************************
// *                                Playlist                                 *
// ***************************************************************************

// Playlist will point to the next value after being processed by
// GetVarintAsOffsetAndInc, which in the case of a playlist is the pattern
// table.
//
// Increases the parent playlist reference counter.
ZSequence::Playlist::Playlist(ZSequence* parent, Stream playlist, 
    Callbacks callbacks) : 
    in_pattern_(false),
    coda_(false),
    repeat_counter_(kRepeatUninitialized),
    return_to_(0),
    playlist_base_(playlist),
    shared_callbacks_(callbacks),
    cursor_(GetVarintAsOffsetAndInc(&playlist)),
    pattern_start_table_(playlist),
    parent_(parent),
    completed_(false) {
  ++(parent_->playlist_refs_);
}

// Decrease the parent playlist reference counter.
ZSequence::Playlist::~Playlist() { --(parent_->playlist_refs_); }

ZSequence::Stream ZSequence::Playlist::GetPatternData(int pattern) {
  const uint16_t offset = 
      *(reinterpret_cast<const uint16_t*>(pattern_start_table_) + pattern);
  return playlist_base_ + offset;
}

StatusOr<bool> ZSequence::Playlist::AdvanceAnyPatternEvent(
    int* read_code_count) {
  bool completed = true;
  do {
    // We read the event like this instead of with a GetUByteAndInc because we
    // will not want to advance the cursor if we intend to call
    // AdvancePatternEvent
    const uint8_t event_code = *cursor_;
    ++(*read_code_count);
    switch (event_code) {
      case PatternEvents::kDelay: {
        ++cursor_;
        const uint16_t delay_ticks = util::varint::GetVarintAndInc(&cursor_);
        shared_callbacks_.rest_callback(delay_ticks);
        return false;
      }
      case PatternEvents::kReturn:
        // Pattern reading has completed
        return true;
    }
    ASSIGN_OR_RETURN(completed, AdvancePatternEvent());
    // Ensure we don't read too many events in the case where the ZSEQ has a
    // stupid format.
  } while (!completed || (*read_code_count < kMaxAdvanceEvents));

  // We must still be reading this pattern
  return false;
}

StatusOr<bool> ZSequence::Playlist::Advance() {
  if (completed_) {
    return util::FailedPreconditionError("Cannot advance a playlist that has "
        "reached its end.");
  }
  for(int read_code_count = 0; read_code_count < kMaxAdvanceEvents; 
      ++read_code_count) {
    if (in_pattern_) {
      ASSIGN_OR_RETURN(bool pattern_complete, 
          AdvanceAnyPatternEvent(&read_code_count));
      if (!pattern_complete) return false; 

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
      case PlaylistEvents::kStop: {
        completed_ = true;
        return true;
      }
      default: 
        return util::FormatMismatchError(StrCat("Unrecognized playlist event. "
            "(", playlist_event, ")"));
    } // switch
  } // for
  return util::FormatMismatchError("Too many non-event sequence codes in "
      "a row.");
}

// ***************************************************************************
// *                            NoteEventPlaylist                            *
// ***************************************************************************

ZSequence::NoteEventPlaylist::NoteEventPlaylist(ZSequence* parent,
    Stream playlist, Callbacks callbacks) : Playlist(parent, playlist, 
        *reinterpret_cast<Playlist::Callbacks*>(&callbacks)),
    callbacks_(callbacks), note_range_(0), velocity_(kVolume100P),
    hold_duration_(0), total_duration_(0) {}

StatusOr<bool> ZSequence::NoteEventPlaylist::AdvancePatternEvent() {
  const uint8_t event_code = GetUByteAndInc(&cursor_);

  if (event_code == NotePatternEvents::kSetNoteRange) {
    // First we get the signed byte, cast it to a singed word, then multiply it
    // by 32
    note_range_ = 
        static_cast<int16_t>(static_cast<int8_t>(GetUByteAndInc(&cursor_))) 
        << 5;

    // We haven't reached an event, so we aren't finished reading yet.
    return false;
  }

  // Get the note code from the low 5 bits: 0 - 31
  int16_t note_code = event_code & 0x1f;
  // Get the articulation type from the high 3 bits: 0 - 7, though there are
  // only 6 modes.
  const uint8_t artc_type = (event_code & 0xe0) >> 5;

  if (artc_type > 5) {
    return util::FormatMismatchError(
      StrCat("Articulation type out of range: (", artc_type, " > 5)"));
  }
  switch (artc_type) {
    // Mode 0: All articulation parameters change
  case 0:
    velocity_ = static_cast<double>(GetUByteAndInc(&cursor_)) / 255.0;
    total_duration_ = util::varint::GetVarintAndInc(&cursor_);
    hold_duration_ = TakeUBytePercentageAndInc(total_duration_, &cursor_);
    break;
    // Mode 1: Change duration and velocity, set hold to 100%
  case 1:
    velocity_ = static_cast<double>(GetUByteAndInc(&cursor_)) / 255.0;
    total_duration_ = util::varint::GetVarintAndInc(&cursor_);
    hold_duration_ = total_duration_;
    break;
    // Mode 2: Only velocity changes.
  case 2:
    velocity_ = static_cast<double>(GetUByteAndInc(&cursor_)) / 255.0;
    break;
    // Mode 3: Change duration and hold, but leave velocity.
  case 3:
    total_duration_ = util::varint::GetVarintAndInc(&cursor_);
    hold_duration_ = TakeUBytePercentageAndInc(total_duration_, &cursor_);
    break;
    // Mode 4: Change duration, set hold to 100%, leave velocity
  case 4:
    total_duration_ = util::varint::GetVarintAndInc(&cursor_);
    hold_duration_ = total_duration_;
    break;
    // Mode 5: Nothing changes
  case 5:
    break;
  default:
    ASSERT(false);
  }

  callbacks_.articulate_callback(note_range_ + note_code, velocity_, 
      hold_duration_, total_duration_);

  // We've reached an actual event, so we're done reading.
  return true;
}

// ***************************************************************************
// *                         ParameterEventPlaylist                          *
// ***************************************************************************

ZSequence::ParameterEventPlaylist::ParameterEventPlaylist(ZSequence* parent,
    Stream playlist, Callbacks callbacks) : Playlist(parent, playlist,
    *reinterpret_cast<Playlist::Callbacks*>(&callbacks)),
    callbacks_(callbacks), pitch_shift_64th_(0) {}

StatusOr<bool> ZSequence::ParameterEventPlaylist::AdvancePatternEvent() {
  uint8_t event_code = GetUByteAndInc(&cursor_);

  // The HO bit indicates that theres also a duration to be read.
  bool read_duration = !(event_code & 0x80);
  // Erase the HO bit to halve the number of codes to check
  event_code &= 0x7f;

  uint16_t duration = 0;
  switch (event_code) {
    case ParameterPatternEvents::kSetVolume: {
      const uint8_t volume = GetUByteAndInc(&cursor_);
      if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
      callbacks_.set_volume_callback(static_cast<double>(volume) / 255.0, 
          duration);
      return true;
    }
    case ParameterPatternEvents::kSetPan: {
      const int8_t pan = GetUByteAndInc(&cursor_);
      if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
      callbacks_.set_pan_callback(static_cast<double>(pan) / 128.0,
          duration);
      return true;
    }
    case ParameterPatternEvents::kSetPitch: {
      const int16_t pitch_shift = GetUWordAndInc(&cursor_);
      if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
      pitch_shift_64th_ = pitch_shift;
      callbacks_.set_pitch_shift_callback(
          static_cast<double>(pitch_shift_64th_) / 64.0, duration);
      return true;
    }
    case ParameterPatternEvents::kAddPitch: {
      const int8_t pitch_shift_offset = GetUByteAndInc(&cursor_);
      if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
      pitch_shift_64th_ += pitch_shift_offset;
      callbacks_.set_pitch_shift_callback(
          static_cast<double>(pitch_shift_64th_) / 64.0, duration);
      return true;
    }
    case ParameterPatternEvents::kSetVibratoRange: {
      const uint8_t vibrato_range = GetUByteAndInc(&cursor_);
      if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
      callbacks_.set_vibrato_range_callback(
          static_cast<double>(vibrato_range) / 16.0, duration);
      return true;
    }
    case ParameterPatternEvents::kSetInstrument: {
      const uint8_t instrument = GetUByteAndInc(&cursor_);
      if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
      callbacks_.set_instrument_callback(instrument, duration);
      return true;
    }
    default:
      return util::FormatMismatchError(StrCat("Unrecognized pattern parameter "
          "event. (", event_code, ")"));
  } // switch
}

// ***************************************************************************
// *                           MasterEventPlaylist                           *
// ***************************************************************************

ZSequence::MasterEventPlaylist::MasterEventPlaylist(ZSequence* parent, 
    Stream playlist, Callbacks callbacks) : Playlist(parent, playlist,
    *reinterpret_cast<Playlist::Callbacks*>(&callbacks)),
    callbacks_(callbacks) {}

StatusOr<bool> ZSequence::MasterEventPlaylist::AdvancePatternEvent() {
  uint8_t event_code = GetUByteAndInc(&cursor_);
  
  // See docs for ParameterEventPlaylist.
  bool read_duration = !(event_code & 0x80);
  event_code &= 0x7f;

  uint16_t duration = 0;
  switch (event_code) {
  case MasterPatternEvents::kSetMasterVolume: {
    const uint8_t volume = GetUByteAndInc(&cursor_);
    if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
    callbacks_.set_master_volume_callback(static_cast<double>(volume) / 255.0,
        duration);
    return true;
  }
  case MasterPatternEvents::kSetMasterPan: {
    const int8_t pan = GetUByteAndInc(&cursor_);
    if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
    callbacks_.set_master_pan_callback(static_cast<double>(pan) / 128.0,
        duration);
    return true;
  }
  case MasterPatternEvents::kSetMasterPitchShift: {
    const int16_t pitch_shift = GetUWordAndInc(&cursor_);
    if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
    callbacks_.set_master_pitch_shift_callback(
        static_cast<double>(pitch_shift) / 64.0, duration);
    return true;
  }
  case MasterPatternEvents::kSetTempo: {
    const uint8_t tempo = GetUByteAndInc(&cursor_);
    if (read_duration) duration = util::varint::GetVarintAndInc(&cursor_);
    callbacks_.set_tempo_callback(tempo, duration);
    return true;
  }
  default:
    return util::FormatMismatchError(StrCat("Unrecognized master parameter "
        "event. (", event_code, ")"));
  } // switch
}

} // namespace audio