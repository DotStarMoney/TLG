#ifndef TLG_LIB_STAGE_H_
#define TLG_LIB_STAGE_H_

#include <memory>
#include <string>
#include <vector>

#include "physics/retro.h"
#include "tlg_lib/rescache.h"
#include "tlg_lib/tileset.h"
#include "util/loan.h"

namespace tlg_lib {

class StageContent : public Loadable {
  friend class ResCache;

 public:
  // some flavor of draw method

 private:
  // Members for ResCache ------------------------------------------------------
  static constexpr uint64_t kTypeId = 0x22b86f3bbce132bb;
  uint64_t type_id() const override { return kTypeId; }
  static std::unique_ptr<StageContent> Load(const std::string& uri,
                                                  ResCache* cache);
  // ---------------------------------------------------------------------------
  StageContent(std::vector<util::Loan<const Tileset>>&& tilesets,
                     std::vector<std::vector<int32_t>>&& layers,
                     physics::retro::BlockGrid&& collision)
      : tilesets_(tilesets), layers_(layers), collision_(collision) {}

  std::vector<util::Loan<const Tileset>> tilesets_;
  std::vector<std::vector<int32_t>> layers_;
  physics::retro::BlockGrid collision_;
};

}  // namespace tlg_lib
#endif  // TLG_LIB_STAGE_H_
