#include "mixer.h"

#include "assert.h"

namespace audio {

Mixer::Mixer(AudioSystem* const parent, Format format) : 
    AudioComponent(parent), SampleSupplier<double>(format) {}

void Mixer::SetInput(unsigned int index, SampleSupplier<double>* input) {
  ASSERT_LT(index, inputs_.size());
  inputs_[index] = input;
}

void Mixer::PushInput(SampleSupplier<double>* input) {
  inputs_.push_back(input);
}

void Mixer::RemoveInput(unsigned int index) {
  ASSERT_LT(index, inputs_.size());
  inputs_.erase(inputs_.begin() + index);
}

void Mixer::PopInput() {
  inputs_.pop_back();
}

void Mixer::ClearInputs() {
  inputs_.clear();
}

util::Status Mixer::ProvideNextSamples(Iter samples_start,
    uint32_t sample_size, uint32_t sample_clock) {
  mix_buffer_.resize(sample_size * inputs_.size());
  // auto pool = GetSchedulePool(inputs_.size());
  for (int input_i = 0; input_i < inputs_.size(); ++input_i) {
    SampleSupplier<double>* const input = inputs_[input_i];
    SampleSupplier<double>::Iter start_buffer = 

    // pool.Schedule([this, sample_size, sample_clock, input_i](){
    //   inputs_[input_i]
    //
    //
    //
  }

  // Add pooling
  // Change "AudioComponent"
  // Add ability to specify stride in sample supplier
  // Remove sample clock

}



} // namespace audio