#ifndef TLG_LIB_STAGE_H_
#define TLG_LIB_STAGE_H_

#include <vector>

#include "physics/retro.h"
#include "tlg_lib/tileset.h"
#include "util/loan.h"

namespace tlg_lib {

class StaticStageContent {

  
 private:
  struct TileDescriptor {
    uint32_t tile_i : 24, set_i : 8;
  };

  std::vector<util::Loan<const Tileset>> tilesets_;
  std::vector<std::vector<TileDescriptor>> layers_;
  physics::retro::BlockGrid collision_;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_STAGE_H_
