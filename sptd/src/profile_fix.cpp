// Fresh profiles: report the game's unwritten save blobs as "default".
//
// At start-up the game reads its three title-specific profile settings
// (XPROFILE_TITLE_SPECIFIC1-3, ids 0x63E83FFF/FFE/FFD), where it keeps its
// progress, through sub_824068F0 (XUserReadProfileSettings: title, user,
// count, ids, &size, buffer, overlapped). The result handler
// (sub_82298038) accepts each setting only when its source is
// XSOURCE_DEFAULT (1, "nothing saved yet") or XSOURCE_TITLE (2) with binary
// data. ReXGlue v0.10.0 answers a setting that was never written with source
// 0 (XSOURCE_NO_VALUE), which the game treats as a corrupt save: it marks the
// signed-in player's slot as failed, and the next lookup of that player
// (sub_82101AF0 via sub_82297C48) dereferences a null pointer at the title
// screen (guest read of 0x1A) whenever a controller is connected.
//
// After the real call, rewrite source 0 to 1 for those three settings. Saved
// blobs (source 2) are untouched. The read completes immediately in ReXGlue,
// so the buffer is filled when the call returns.

#include <cstdint>

#include <rex/hook.h>
#include <rex/logging.h>

namespace {

uint32_t Load32(const uint8_t* base, uint32_t address) {
  const uint8_t* p = base + address;
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  uint8_t* p = base + address;
  p[0] = uint8_t(value >> 24);
  p[1] = uint8_t(value >> 16);
  p[2] = uint8_t(value >> 8);
  p[3] = uint8_t(value);
}

bool IsTitleSpecific(uint32_t id) { return id >= 0x63E83FFD && id <= 0x63E83FFF; }

}  // namespace

REX_EXTERN(__imp__sub_824068F0);
REX_HOOK_RAW(sub_824068F0) {
  const uint32_t buffer = ctx.r8.u32;
  __imp__sub_824068F0(ctx, base);
  if (!buffer) return;  // size query
  // XUSER_READ_PROFILE_SETTING_RESULT { count, settings* }; each
  // XUSER_PROFILE_SETTING is 0x28 bytes: source at +0, id at +0x10.
  const uint32_t count = Load32(base, buffer);
  const uint32_t settings = Load32(base, buffer + 4);
  if (!settings || count > 64) return;
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t setting = settings + i * 0x28;
    if (Load32(base, setting) == 0 && IsTitleSpecific(Load32(base, setting + 0x10))) {
      Store32(base, setting, 1);  // XSOURCE_DEFAULT
      REXLOG_INFO("SPTD: profile setting {:08X} not saved yet, reported as default", Load32(base, setting + 0x10));
    }
  }
}
