#include <memory>

#include "glog/logging.h"
#include "retro/fbgfx.h"
#include "retro/fbimg.h"
#include "tlg_lib/indexstagegraph.h"

using retro::FbColor32;
using retro::FbGfx;
using retro::FbImg;

using tlg_lib::StageGraph;


int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  /*
  FbGfx::Screen({320, 240}, false, "FbGfx", {1080, 720});

  FbGfx::Cls(FbColor32::BLACK);

  auto testImg = FbImg::FromFile("res/wodde.png");

  FbGfx::Put(*testImg.get(), {0, 0});

  FbGfx::Rect({0, 0}, {100, 200});
  FbGfx::TextParagraph("The Supple Husk Expelled Sweet Egg Wax", {0, 0},
                       {100, 200});

  FbGfx::Flip();

  system("pause");
  */
  return 0;
}
