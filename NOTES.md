# Technical notes

South Park: Let's Go Tower Defense Play! (XBLA, title `58410931`, `default.xex` 0.0.1.2, 8,499,200 bytes,
CRC32 `A21E2AF7`) on the prebuilt ReXGlue SDK v0.10.0, with no SDK patches. Analysis data that comes from the game
(images, dumps, generated code) stays local and is git-ignored.

Prior art: [superheher/south-park-tower-defence-recomp](https://github.com/superheher/south-park-tower-defence-recomp)
(MIT) ported the game to Linux on ReXGlue v0.8.0 with 19 SDK patches. This port starts again from v0.10.0, which
already contains most of what those patches fixed, and reuses that project's findings where they still apply:
the setjmp/longjmp addresses, its list of indirectly called functions (`sptd/missing_funcs.toml`), the
XUsbcam stubs and the Present function.

## Code generation

- `generate_exception_handlers = true`: the game uses Win32 SEH. The generated PCH of v0.10.0 does not include
  `rex/platform/exceptions.h` (which defines `SEH_TRY`/`SEH_CATCH_ALL`/`SEH_END`) or, on Windows, `<excpt.h>`
  (`GetExceptionCode`). `src/seh_prelude.h` is force-included into the recompiled sources for that.
- `setjmp_address = 0x8242EEA0`, `longjmp_address = 0x8242EA70`: the image loader tries each decoder under
  setjmp and a failed decode longjmps back (found by the Linux port).
- `sub_824A0960` is given its real size (`0x578`): the analyzer ended it at `0x824A0DA4` while its branches reach
  `0x824A0E10..0x824A0EC8`, which left undeclared labels.
- The remaining "unresolved branch" lines in `codegen.log` (11) do not stop the build; none has been reached in
  testing so far.

## Imports

`XUsbcam*` (Xbox Live Vision camera) are not exported by the runtime. `src/stubs.cpp` returns `E_FAIL`, as a
console without a camera would.

## Crash at the title screen with a controller connected (fixed)

**Symptom:** guest read of `0x1A` in `sub_82101AF0` a few seconds after the title screen, but only when a
controller is connected.

**Cause:** at start-up the game reads its three title-specific profile settings (`0x63E83FFF/FFE/FFD`, its save
data) with `XUserReadProfileSettings` (`sub_824068F0`). The result handler (`sub_82298038`) accepts a setting
only when its source is `XSOURCE_DEFAULT` (1) or `XSOURCE_TITLE` (2) with binary data of the expected total size.
ReXGlue reports a never-written setting with source 0, so the game marks the player's slot as failed (state 2
in the per-user block at `0x828EB348 + user * 0xC68 + 0x954`), and `sub_82297C48` then returns null for the
signed-in player.

**Fix:** `src/profile_fix.cpp` hooks `sub_824068F0` and reports those three unwritten settings as source 1.
Saved data (source 2) is untouched. Earthworm Jim HD's fix for the same runtime behaviour (seeding empty binary
blobs) would not work here: this game requires the blobs' total size to match, so an empty blob is also
rejected.

## Frame rate

The game advances one simulation step per frame and paces itself to the console's 60 Hz vblank. Guest `vsync`
is forced on (`settings.cpp`); with it off the game runs as fast as the PC can draw. There is deliberately no
frame-rate setting. The F3 overlay's "FPS" is the host presenter; `Guest FPS` in the log (from the Present hook,
`sub_821BFF48`) is the game's own rate and measured 60.0 in menus.

## Launcher

TRG Launcher 1.1.0 (`third_party/trg-launcher`), integrated like Earthworm Jim HD: install from the player's own
STFS package with title ID and `default.xex` CRC checks, display/graphics/gameplay/controls pages, achievements
(names and icons read from the player's `default.xex` on first play), updater, F11 / Alt+Enter fullscreen.

Controller remapping hooks `sub_82448838` (the title's `XInputGetState` stub: a branch to
`XamInputGetState(user, flags, state)`, so the state pointer is `r5`) and `sub_82448840` (`XInputSetState`).

## Developer aids

- `--sptd_null_trace=true` logs a symbolized host stack when guest code touches the guest null page (Windows).
- `SPTD_DUMP_IMAGE=<file>` writes the decrypted guest image; `tools/ppc_dis.py` disassembles it.
- `sptd-dev` preset builds `src/dev_trace.cpp` (hooks used while finding the profile bug).
- `tools/dump_stack.py` symbolizes a crash dump's stack without a debugger.

## Not done yet / known issues

- Gameplay has had short tests: menus, game modes, the opening movie and the first waves of the school-yard
  level. A full play-through, saving and reloading, and the other modes still need testing.
- The game also asks for `Media\Assets\Movies\en-en\sp_xbox_0_intro.wmv`, which the package doesn't have, and
  `Frontend\Graphics\en-en\*.png`; it carries on without them (the opening movie still plays).
- The launcher's title-screen art is taken from the first non-dark frame from 32 s after the first frame (the
  game shows black for ~30 s at start); the first version captured a black frame at 20 s.
- On the game-mode screen, the locked "HARD" entry draws with garbled letters; not yet checked against a console.
- Online play (Xbox LIVE game mode) is not supported.
