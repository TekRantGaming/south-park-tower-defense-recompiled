// Developer-only tracing hooks (SPTD_DEV_TOOLS builds). Never in release builds.

#include <cstdint>

#include <rex/hook.h>
#include <rex/logging.h>

namespace {
uint32_t Load32(const uint8_t* base, uint32_t address) {
  const uint8_t* p = base + address;
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
constexpr uint32_t kUsers = 0x828EB348;  // per-user blocks, 3176 bytes each
}  // namespace

REX_EXTERN(__imp__sub_82101AF0);
REX_HOOK_RAW(sub_82101AF0) {
  const uint32_t user = ctx.r3.u32;
  const uint32_t mask = Load32(base, kUsers + 1600);
  const uint32_t state = user < 4 ? Load32(base, kUsers + user * 3176 + 2388) : 0xFFFFFFFF;
  static uint32_t last = 0xFFFFFFFF;
  if (((mask << 8) | state | (user << 16)) != last) {
    last = (mask << 8) | state | (user << 16);
    REXLOG_INFO("SPTD trace: sub_82101AF0(user {}) mask {:X} state {} lr {:08X}", user, mask, state, uint32_t(ctx.lr));
  }
  __imp__sub_82101AF0(ctx, base);
}

REX_EXTERN(__imp__sub_82297F30);
REX_HOOK_RAW(sub_82297F30) {
  const uint32_t user = ctx.r3.u32;
  __imp__sub_82297F30(ctx, base);
  REXLOG_INFO("SPTD trace: sub_82297F30(user {}) -> {:X}, state {}", user, ctx.r3.u32,
              user < 4 ? Load32(base, kUsers + user * 3176 + 2388) : 0);
}

REX_EXTERN(__imp__sub_82297B40);
REX_HOOK_RAW(sub_82297B40) {
  REXLOG_INFO("SPTD trace: sub_82297B40(r3 {:X}) lr {:08X}", ctx.r3.u32, uint32_t(ctx.lr));
  __imp__sub_82297B40(ctx, base);
}
