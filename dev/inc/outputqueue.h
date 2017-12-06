#ifndef AUDIO_OUTPUTQUEUE_H_
#define AUDIO_OUTPUTQUEUE_H_

#include <type_traits>
#include <vector>

#include "assert.h"
#include "status.h"
#include "audio_format.h"

namespace audio {
 
 // An interface for queueing audio onto a playback device.
class OutputQueue {
 public:
  virtual ~OutputQueue() {}

  // Return the number of samples left to be played back in the audio queue.
  virtual int64_t GetQueuedSamplesSize() const = 0;

  // Queue a number of samples for playback.
  template <class T>
  util::Status QueueSamples(T* samples, int64_t samples_size) {
    ASSERT(IsValidSampleType<T>());
    return QueueBytes(samples, samples_size * sizeof(T));
  }
  template <class T>
  util::Status QueueSamples(const std::vector<T>& samples) {
    ASSERT(IsValidSampleType<T>());
    return QueueBytes(samples.data(), samples.size * sizeof(T));
  }
  Format format() const { return format_; }
 protected:
  OutputQueue(Format format) : format_(format) {}
  const Format format_;

  // Queue bytes of data onto the audio device.
  virtual util::Status QueueBytes(void* data, int64_t data_size) = 0;

 private:
  template <class T>
  bool IsValidSampleType() {
    switch(format_.sample_format) {
      case INT8:
        return std::is_same<T, int8_t>::value;
      case INT16:
        return std::is_same<T, int16_t>::value;
      case INT32:
        return std::is_same<T, int32_t>::value;
      case INT64:
        return std::is_same<T, int64_t>::value;
      case FLOAT32:
        return std::is_same<T, float>::value;
      case FLOAT64:
        return std::is_same<T, double>::value;
      default:
        ASSERT(false);
    }
  }
};

} // namespace audio

#endif // AUDIO_OUTPUTQUEUE_H_