// South Park: Let's Go Tower Defense Play! port settings.
//
// Port-specific options are cvars in the "SPTD" categories, so they load and
// save with the rest of ReXGlue's config (south_park_td.toml next to the
// exe) and can be overridden on the command line (--sptd_show_fps=true).

#pragma once

#include <array>
#include <filesystem>
#include <cstdint>
#include <string>
#include <string_view>

#include <rex/cvar.h>

REXCVAR_DECLARE(bool, sptd_launcher);
REXCVAR_DECLARE(bool, sptd_skip_launcher);
REXCVAR_DECLARE(std::string, sptd_updates);
REXCVAR_DECLARE(std::string, sptd_render_quality);
REXCVAR_DECLARE(bool, sptd_show_fps);
REXCVAR_DECLARE(int32_t, sptd_deadzone);
REXCVAR_DECLARE(int32_t, sptd_camera_sensitivity);
REXCVAR_DECLARE(bool, sptd_achievement_toasts);
REXCVAR_DECLARE(bool, sptd_achievement_sound);
REXCVAR_DECLARE(std::string, sptd_achievement_sound_file);
REXCVAR_DECLARE(int32_t, sptd_achievement_volume);
REXCVAR_DECLARE(bool, sptd_vibration);
REXCVAR_DECLARE(int32_t, sptd_vibration_strength);
REXCVAR_DECLARE(bool, sptd_invert_rs_x);
REXCVAR_DECLARE(bool, sptd_invert_rs_y);
REXCVAR_DECLARE(bool, sptd_invert_ls_x);
REXCVAR_DECLARE(bool, sptd_invert_ls_y);

namespace sptd {

// Xbox 360 gamepad buttons, in XINPUT_GAMEPAD wButtons bit order where they
// are buttons; the triggers are analog in the gamepad state and are handled as
// full press / release when remapped.
enum class Pad : uint8_t {
  kDpadUp, kDpadDown, kDpadLeft, kDpadRight,
  kStart, kBack, kLeftThumb, kRightThumb,
  kLeftShoulder, kRightShoulder,
  kA, kB, kX, kY,
  kLeftTrigger, kRightTrigger,
  kCount,
  kNone = 0xFF,
};
constexpr size_t kPadCount = static_cast<size_t>(Pad::kCount);

struct PadInfo {
  const char* id;     // config value / cvar suffix, e.g. "a" -> sptd_map_a
  const char* label;  // UI label
  uint16_t mask;      // XINPUT_GAMEPAD bit, 0 for triggers
};
const PadInfo& GetPadInfo(Pad pad);
Pad ParsePad(std::string_view id);  // kNone for "none"/unknown

// What each physical control sends to the game (sptd_map_<id>).
Pad GetMapping(Pad physical);
void SetMapping(Pad physical, Pad target);

// Render-resolution presets (sptd_render_quality).
struct RenderPreset {
  const char* id;
  const char* label;
  double ratio;  // output size / render size
};
const std::array<RenderPreset, 6>& RenderPresets();
// resolution_scale (1-8) a preset gives for an output height; 0 for "custom".
int RenderScaleFor(std::string_view preset, int output_height);
// Applies sptd_render_quality for the given output height (call before the GPU starts).
void ApplyRenderPreset(int output_height);

// Writes the current settings (skipping command-line/env overrides) to `path`.
bool SaveSettings(const std::filesystem::path& path);

// Changes a cvar's default; values from the config file or command line still win.
void SetCvarDefault(std::string_view name, std::string_view value);

// Port defaults that differ from ReXGlue's (call before the config is loaded).
void ApplyPortDefaults();

// Settings the port sets itself at every start (never saved or reset): the
// guest video mode, kept at 1280x720 whatever the window size, and the
// runtime settings ApplyRuntimeOverrides forces.
bool IsPinnedCvar(std::string_view name);

// Forces the ReXGlue settings the port depends on (see settings.cpp).
void ApplyRuntimeOverrides();

}  // namespace sptd
