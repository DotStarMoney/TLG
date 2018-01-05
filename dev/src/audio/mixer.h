#ifndef AUDIO_MIXER_H_
#define AUDIO_MIXER_H_

#include <vector>

#include "samplesupplier.h"
#include "audio_format.h"
#include "audiosystem.h"

namespace audio {

class Mixer : public AudioComponent, public SampleSupplier<double>, {
 public:
  Mixer(AudioSystem* const parent, Format format);

  // Set the input at the provided input index. The input at the index must
  // exist.
  void SetInput(unsigned int index, SampleSupplier<double>* input);

  // Add an input to the end of the inputs list.
  void PushInput(SampleSupplier<double>* input);
  
  // Remove an input from the input list at the index. All inputs above the
  // index move down to fill the space. The input at the index must exist.
  void RemoveInput(unsigned int index);

  // Remove the last input from the input list if it exists.
  void PopInput();

  // Clear the input list.
  void ClearInputs();

  unsigned int inputs_size() const { return inputs_.size(); }

  util::Status ProvideNextSamples(
      Iter samples_start,
      uint32_t sample_size,
      uint32_t sample_clock) override;

 private:
  std::vector<SampleSupplier<double>*> inputs_; // inputs are not owned.
  std::vector<double> mix_buffer_;
};

} // namespace audio

#endif // AUDIO_MIXER_H_