#ifndef RETRO_FBGFX_H_

#include <memory>

#include "glm/vec2.hpp"

namespace retro {



class FbGfx final {
 public:
  static std::unique_ptr<FbGfx> CreateScreen(glm::ivec2 dimensions);

  // suite of draw functions, will also need some rudamentary SDL image wrapper.

  void WaitForVSync() const;

  void Flip();

 private:
  FbGfx(glm::ivec2 dimensions);
};

}  // namespace retro

#endif  // RETRO_FBGFX_H_
