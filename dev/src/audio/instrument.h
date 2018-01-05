#ifndef AUDIO_INSTRUMENT_H_
#define AUDIO_INSTRUMENT_H_

#include <vector>

#include "instrument_characteristics.h"
#include "resourcemanager.h"

namespace audio {

// A resource for an instrument that maps different samples and playback
// characteristics over a range of semitone aligned ranges.
//
// Instruments can only be created by Deserializing byte streams of the
// following format:
//
// (all header numbers are considered unsigned unless postfixed with an S)
//
// BYTES |                 VALUE
// ==============================================
//     4 |                           ASCII "TLGR"
// ______________________________________________
//     4 |                           ASCII "INST"
// ______________________________________________
//     1 |                            # of splits
// ______________________________________________
//
// FOR EACH SPLIT {
// --------------
//
// NOTE: Semitone offsets MUST be in INCREASING order
// ______________________________________________
//     8 |              resource ID of the sample
// ______________________________________________
//    2S | inclusive semitone offset for start of 
//       |                                  split
// ______________________________________________
//     1 |   mode: bit 1 is set if looping, bit 2
//       |                     is set if envelope
// ______________________________________________
//    *4 |     if loop: inclusive sample index of
//       |                             loop start
// ______________________________________________
//    *4 |     if loop: inclusive sample index of
//       |                               loop end
// ______________________________________________
//    *2 |              if envelope: attack in ms
// ______________________________________________
//    *2 |               if envelope: decay in ms
// ______________________________________________
//    *1 |  if envelope: sustain as a % from 0 to 
//       |                                    255             
// ______________________________________________
//    *2 |             if envelope: release in ms
// ______________________________________________
//
// }
//
class Instrument : public Resource {
 public:
  static constexpr int64_t kResourceUID = 0x349F7B23AD07BADD;
  static util::StatusOr<std::unique_ptr<const Resource>> 
      Deserialize(std::istream* stream);

  int64_t resource_uid() const override { return kResourceUID; }
  int64_t GetUsageBytes() const override;

  // We must define this as CharacteristicInfo pointers may need cleaning up.
  ~Instrument();

  // Sample ADSR envelope and loop chracteristics. We seperate these from the
  // split data as some splits may have neither of these specified and we can
  // avoid allocating space for them in this case.
  //
  // When the serialized form of an instrument does not specify an envelope or
  // loop, the defaults from InstrumentCharacteristics are assumed.
  class CharacteristicInfo {
   friend class Instrument;
   public:
    const InstrumentCharacteristics::ADSRSeconds envelope_;
    const InstrumentCharacteristics::LoopInfo loop_;
   private:
     CharacteristicInfo(InstrumentCharacteristics::ADSRSeconds envelope,
        InstrumentCharacteristics::LoopInfo loop) : envelope_(envelope), 
        loop_(loop) {}
  };
  // The actual split information.
  struct SplitData {
   friend class Instrument;
   public:
    // Optional CharacteristicInfo, if = nullptr then there is no character and
    // the default values in InstrumentCharacteristics are assumed.
    const CharacteristicInfo *const character_;
    // The offset in semitones that samples played in this range should be
    // adjusted by. This is so that a sample of higher pitch used to add
    // fidelity for higher semitone offsets will not automatically play back at
    // the higher playback pitch on top of its already high pitch.
    const int16_t base_offset_;
    // An index passed into GetSampleID to get the resource id of the sample
    // data this split references.
    const uint8_t sample_index_;
   private:
    SplitData(const CharacteristicInfo *const character, int16_t base_offset,
        uint8_t sample_index) : character_(character), 
        base_offset_(base_offset), sample_index_(sample_index) {}
  };

  // Provided an offset in semitones, get the split data for that offset.
  const SplitData& GetPlayCharacteristics(double semitones) const;
  // Provided a sample_index from a call to GetPlayCharacteristics, get a 
  // resource id for the sample referenced by the split.
  //
  // We bother doing things this way so that a user of Instrument can create
  // their own table of sample pointers, and won't have to look up a resource
  // id every time they wish to articulate a new sample from a this instrument.
  const ResourceManager::MapID& GetSampleID(uint8_t index) const;

 private:
  // Lack of a useful constructor indicates that Instruments should only be
  // created through Deserialization.
  Instrument(int64_t total_bytes) : total_bytes_(total_bytes) {}

  // A mapping from indexes to resource IDs.
  std::vector<ResourceManager::MapID> res_ids_;
  // Clients querying splits_ should not re-acquire the split when repeating a
  // note as we must call splits_.lower_bound: triggering a binary search for
  // every invocation.
  std::vector<SplitData> splits_;

  // The cached number of bytes taken up by this instrument, computed during
  // deserialization.
  const int64_t total_bytes_;
};

} // namespace audio

#endif // AUDIO_INSTRUMENT_H_