#ifndef RETRO_FBCORE_H_

#include <stdint.h>

namespace retro {

struct FbColor32 {
  union {
    struct {
      unsigned int a : 8;
      unsigned int b : 8;
      unsigned int g : 8;
      unsigned int r : 8;
    } channel;
    int32_t value;
  };
  FbColor32& operator=(const int32_t& x) { this->value = x; }
  FbColor32(const int32_t& x) { this->value = x; }
  operator int32_t() const { return this->value; }
};

}  // namespace retro

#endif  // RETRO_FBCORE_H_
