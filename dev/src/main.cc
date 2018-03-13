#include "glog/logging.h"
#include "retro/fbgfx.h"
#include "retro/fbimg.h"

using retro::FbColor32;
using retro::FbGfx;
using retro::FbImg;

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  FbGfx::Screen({640, 480});

  FbGfx::Cls(FbColor32::BLACK);

  auto testImg = FbImg::FromFile("res/wodde.png");

  FbGfx::Put(*testImg.get(), {0, 0});

  FbGfx::Rect({0, 0}, {100, 200});
  FbGfx::TextParagraph("The Supple Husk Belched Sweet Egg Wax", {0, 0},
                       {100, 200});

  FbGfx::Flip();

  system("pause");

  return 0;
}
