#ifndef TLG_LIB_STAGE_H_
#define TLG_LIB_STAGE_H_

#include <vector>

#include "physics/retro.h"
#include "util/loan.h"
#include "retro/fbimg.h"

namespace tlg_lib {

class StaticStageContent {
  typedef uint32_t TileDescriptor;

  std::vector<util::Loan<retro::FbImg>> l;
  std::vector<std::vector<TileDescriptor>> layers;
  physics::retro::BlockGrid collision;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_STAGE_H_
