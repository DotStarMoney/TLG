#include "experimental/radarconcept.h"

#include <limits>

#include "absl/strings/substitute.h"
#include "glm/geometric.hpp"
#include "physics/geometry2.h"
#include "retro/fbcore.h"
#include "retro/fbgfx.h"
#include "retro/fbimg.h"
#include "util/random.h"

using glm::dvec2;
using glm::ivec3;
using physics::geometry2::Line2;
using physics::geometry2::Ray2;
using retro::FbColor32;
using retro::FbGfx;
using retro::FbImg;
using std::vector;

namespace experimental {
namespace radarconcept {

enum Mode { DRAW, RADAR };
enum DrawPrimitive {
  NONE,
  POINT,
  LINE,
};

constexpr uint32_t kScreenW = 320;
constexpr uint32_t kScreenH = 240;

const dvec2 kHalfScreen(kScreenW / 2, kScreenH / 2);

constexpr double kCameraSpeed = 1.0;
constexpr double kRadarSpeed = 0.1;

int run() {
  FbGfx::Screen({kScreenW, kScreenH}, false, "TLG Radar Test", {1024, 768});

  Mode mode = DRAW;
  bool drawing_line = false;

  dvec2 camera{0, 0};
  auto final_img = FbImg::OfSize({ kScreenW, kScreenH });


  dvec2 radar[] = {{0, 0}, {0, 0}};
  bool have_point = false;
  DrawPrimitive radar_can_draw = NONE;
  double theta = 0;

  auto black_img = FbImg::OfSize({kScreenW, kScreenH});
  FbGfx::Cls(*black_img, FbColor32::BLACK);
  auto radar_dest_img = FbImg::OfSize({kScreenW, kScreenH});

  vector<Line2> lines;
  uint32_t ring_start_index = 0;

  bool spacebar[] = {false, false};
  FbGfx::MouseButtonPressedState button_states[] = {{false, false, false},
                                                    {false, false, false}};
  bool backspace[] = {false, false};
  while (!FbGfx::Close() && !FbGfx::GetKeyPressed(FbGfx::ESCAPE)) {
    FbGfx::SyncInputs();

    spacebar[1] = spacebar[0];
    spacebar[0] = FbGfx::GetKeyPressed(FbGfx::SPACEBAR);
    const bool space_pressed = spacebar[0] && !spacebar[1];

    backspace[1] = backspace[0];
    backspace[0] = FbGfx::GetKeyPressed(FbGfx::BACKSPACE);
    const bool backsapce_pressed = backspace[0] && !backspace[1];

    const auto mouse_state = FbGfx::GetMouse();
    button_states[1] = button_states[0];
    button_states[0] = std::get<1>(mouse_state);
    const bool left_click = button_states[0].left && !button_states[1].left;
    const bool right_click = button_states[0].right && !button_states[1].right;
    const ivec3 mouse_p = std::get<0>(mouse_state) -
                          ivec3{kHalfScreen.x, kHalfScreen.y, 0.0} +
                          ivec3{camera.x, camera.y, 0.0};

    if (!drawing_line && space_pressed) {
      mode = mode == DRAW ? RADAR : DRAW;
    }

    double dist = std::numeric_limits<double>::max();
    Ray2 ray;
    if (mode == DRAW) {
      if (!drawing_line) {
        if (backsapce_pressed && !lines.empty()) lines.pop_back();
        if (left_click) {
          drawing_line = true;
          ring_start_index = lines.size();
          lines.push_back(
              Line2({mouse_p.x, mouse_p.y}, {mouse_p.x, mouse_p.y}));
        }
      } else {
        if (right_click) {
          if ((lines.size() - ring_start_index) >= 3) {
            lines.back().Update(lines.back().start(),
                                lines[ring_start_index].start());
          } else {
            lines.resize(ring_start_index);
          }
          drawing_line = false;
        } else {
          lines.back().Update(lines.back().start(), {mouse_p.x, mouse_p.y});
          if (left_click) {
            lines.push_back(
                Line2({mouse_p.x, mouse_p.y}, {mouse_p.x, mouse_p.y}));
          }
        }
      }
    } else if (mode == RADAR) {
      ray.Update(camera, {std::cos(theta), std::sin(theta)});
      dvec2 closest_intersect = {0, 0};
      for (const auto& line : lines) {
        const auto [intersect, p] =
            physics::geometry2::Operations2::Intersects(ray, line);
        if (intersect) {
          const double this_dist = glm::length(p - camera);
          if (this_dist < dist) {
            dist = this_dist;
            closest_intersect = p;
          }
        }
      }
      if (dist != std::numeric_limits<double>::max()) {
        radar[1] = radar[0];
        radar[0] = closest_intersect;
        radar_can_draw = have_point ? LINE : NONE;
        have_point = true;
      } else {
        radar_can_draw = have_point ? POINT : NONE;
        have_point = false;
      }
      theta += kRadarSpeed;
    }

    camera = camera + dvec2{
        (FbGfx::GetKeyPressed(FbGfx::LEFT_ARROW) ? -kCameraSpeed : 0) +
            (FbGfx::GetKeyPressed(FbGfx::RIGHT_ARROW) ? kCameraSpeed : 0),
        (FbGfx::GetKeyPressed(FbGfx::UP_ARROW) ? -kCameraSpeed : 0) +
            (FbGfx::GetKeyPressed(FbGfx::DOWN_ARROW) ? kCameraSpeed : 0)};

    /////////////////////////////////////////////////////////////////////// Draw
    FbGfx::Cls(*final_img);
    if (mode == DRAW) {
      for (const auto& line : lines) {
        FbGfx::Line(*final_img, 
                    line.start() - camera + kHalfScreen,
                    line.end() - camera + kHalfScreen);
      }
    } else if (mode == RADAR) {
      FbGfx::PutEx(
          *radar_dest_img,
          *black_img,
          {0, 0},
          FbGfx::PutOptions()
          .SetBlend(FbGfx::PutOptions::BLEND_ALPHA)
          .SetMod(FbColor32{255, 255, 255, 8}));
      if (radar_can_draw == POINT) {
        FbGfx::PSet(
            *radar_dest_img, 
            radar[0] - camera + kHalfScreen, 
            FbColor32(0, 200, 255, 255));
      } else if (radar_can_draw == LINE) {
        FbGfx::Line(
            *radar_dest_img,
            radar[0] - camera + kHalfScreen,
            radar[1] - camera + kHalfScreen, 
            FbColor32(0, 200, 255, 255));
      }
      FbGfx::Put(*final_img, *radar_dest_img, {0, 0});
      const double radar_col_s = 0.25 + util::rndd() * 0.75;
      /*
      FbGfx::Line(
          *final_img,
          ray.start() - camera + kHalfScreen, 
          ray.start() - camera + kHalfScreen 
              + ray.delta() * 
                  (dist == std::numeric_limits<double>::max() ? 1000 : dist),
          FbColor32(0, 200 * radar_col_s, 255 * radar_col_s, 255));
      */
    }
    FbGfx::PSet(*final_img, kHalfScreen);
    FbGfx::Put(*final_img, {0, 0});
    FbGfx::TextLine(mode == DRAW ? "DRAW" : "RADAR", {0, 0});
    FbGfx::Flip();
  }
  return 0;
}
}  // namespace radarconcept
}  // namespace experimental
