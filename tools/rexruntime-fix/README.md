# Fix: antivirus false positive on ReXGlue v0.10.0 `rexruntime.dll`

Hand this file to a Claude session working on another ReXGlue port. It explains the problem and the fix,
done first and verified in Earthworm Jim HD v0.9.4.

## The problem
The official ReXGlue SDK **v0.10.0** Windows `bin/rexruntime.dll`
(SHA-256 `e359209fb2b0570e693c966d4c1d99a82465d36ef70d033833fae56adb2f1b7a`) is flagged by about 28/71
VirusTotal engines, including Microsoft Defender (`Trojan:Win32/Wacatac.B!ml`). It is a machine-learning false
positive, but Defender blocks or quarantines any download that contains it. Players think the project was hacked.
Upstream report: https://github.com/rexglue/rexglue-sdk/issues/485

## The fix
`rexruntime.dll` in this folder was built from the **same v0.10.0 source** (tag commit f5337cdc), unmodified.
SHA-256 `e87c3555602c41b18579ef8932639f7afc9324b0ca448f6e7608010d10edbbc3`. No engine flags it:
https://www.virustotal.com/gui/file/e87c3555602c41b18579ef8932639f7afc9324b0ca448f6e7608010d10edbbc3

It exports every function a game built against the official v0.10.0 SDK imports, so the game does **not** need
recompiling: only the DLL is swapped.

**Only replace `rexruntime.dll`. Keep the official `rexgpu-xenos.dll`** (it is clean, 0/57). A rebuilt
xenos plugin made the game abort at startup (0xc0000409).

## Steps for a port
1. **Check the SDK version.** This DLL is only for ReXGlue **v0.10.0**. Hash the project's current
   `rexruntime.dll`. If it is `e359209f...`, use this folder's DLL. If it's another SDK version, don't use it:
   check that version's DLL on VirusTotal by hash, and if it is flagged, rebuild it from that version's tag
   (see "Rebuilding" below).
2. **Verify the imports.** Every `rexruntime.dll` import of the game's exe must be exported by the new DLL
   (use `llvm-readobj --coff-imports` on the exe and `--coff-exports` on the DLL from VS 2022 Build Tools'
   `VC/Tools/Llvm/x64/bin`). Don't use Python's `pefile` for this: it stops reading after 8192 exports.
3. **Put the DLL where builds pick it up.** Copy it over the project's local SDK copy (e.g.
   `tools/rexglue/win-amd64/bin/rexruntime.dll`), because builds copy the SDK DLL next to the exe on every
   build. Keep a backup of the official one.
4. **Rebuild the release, then smoke-test it.** Run the game until it is in game and rendering (check the log for steady
   frame-rate lines and no crash in Windows' Application event log).
5. **Bump the patch version and package the release** the way the project normally does. Before you zip, check
   the zip contains the `e87c3555...` DLL and the official xenos DLL.
6. **Check the zip on VirusTotal.** Its upload needs a CAPTCHA, so ask the user to upload the zip and send the
   link. Don't publish until it's clean. A local Defender scan is not a valid test: it passes even the flagged
   DLL, because the `!ml` verdict comes from Microsoft's cloud check on downloads.
7. **Publish, then document.** Note it in the release notes ("ships rexruntime.dll rebuilt from the same
   ReXGlue v0.10.0 source; antivirus false positive, rexglue/rexglue-sdk#485") and in the project's notes. Linux
   and macOS builds are not affected.

## Rebuilding from source (only for a different SDK version)
- `git clone --branch <tag> --recurse-submodules --shallow-submodules --depth 1
  https://github.com/rexglue/rexglue-sdk.git C:\rexsrc` (keep the path short).
- Windows checks git symlinks out as small text files, and `thirdparty/libmspack/cabextract/mspack/lzxd.c` fails
  to compile. Replace every symlink stub (from `git ls-files -s`, mode 120000, in the repo and all submodules)
  with a copy of its target. The MoltenVK ones are macOS-only and can be skipped.
- In a VS 2022 x64 developer environment with `VC\Tools\Llvm\x64\bin` on PATH: `cmake --preset win-amd64`,
  then `cmake --build out/build/win-amd64 --config Release --target install --parallel`.
- The install step registers the build in `HKCU\Software\Kitware\CMake\Packages\rexglue`. Delete that key so
  other projects don't pick up this SDK by accident.
