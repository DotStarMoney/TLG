#ifndef RETRO_FBCORE_H_
#define RETRO_FBCORE_H_

#include <stdint.h>

namespace retro {

struct FbColor32 {
  FbColor32(int32_t x) : value(x) {}
  FbColor32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    channel.a = a;
    channel.b = b;
    channel.g = g;
    channel.r = r;
  }
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
  operator int32_t() const { return this->value; }
  enum : uint32_t {
    TRANSPARENT_BLACK = 0x00000000,
    BLACK = 0x000000ff,
    WHITE = 0xffffffff,
    RED = 0xff0000ff,
    GREEN = 0x00ff00ff,
    BLUE = 0x0000ffff,
    CYAN = 0x00ffffff,
    MAGENTA = 0xff00ffff,
    YELLOW = 0xffff00ff
  };
};

}  // namespace retro

#endif  // RETRO_FBCORE_H_
