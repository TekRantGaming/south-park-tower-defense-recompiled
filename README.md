# South Park: Let's Go Tower Defense Play! — PC Port

The Xbox 360 game running natively on Windows and Linux: its original code is translated to C++ by static
recompilation with the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) v0.10.0. It comes with the
[TRG launcher](https://github.com/TekRantGaming/trg-launcher). **No game files are included**: you need your own
Xbox Live Arcade package.

| | |
| --- | --- |
| Title ID | `58410931` (Xbox Live Arcade, content type `000D0000`) |
| `default.xex` | version 0.0.1.2 (2009-09-09), 8,499,200 bytes, CRC32 `A21E2AF7` |

## Playing

- **Windows:** unzip `SouthParkTD-<version>-windows-x64.zip`, run `south_park_td.exe`, then **Install from
  package...** on the Play page and pick your package. Press **PLAY**.
- **Linux / Steam Deck:** make `SouthParkTD-<version>-linux-x86_64.AppImage` executable, run it and install the same
  way. Game files go in a `game` folder next to the AppImage.

The launcher has Display, Graphics (render resolution, anti-aliasing, filtering), Gameplay (language), Controls
(remapping, deadzone, vibration, keyboard bindings), Achievements and About pages. F11 or Alt+Enter toggles
fullscreen in game. Hold Shift at start to show the launcher if you turned it off.

## Building

Needs Visual Studio 2022 Build Tools (C++ and its clang), and your own package.

```bash
git submodule update --init
```

```powershell
.\setup.ps1 -Package "<path to your 58410931 package>"
sptd\build.bat
.\tools\package_windows.ps1 -Version v0.1.0
```

Linux AppImage (from WSL Ubuntu 24.04 or Linux, after `setup.ps1`): `tools/build_appimage.sh v0.1.0`.

See [NOTES.md](NOTES.md) for the technical details and what has and hasn't been tested.

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) (BSD 3-Clause).
- [superheher/south-park-tower-defence-recomp](https://github.com/superheher/south-park-tower-defence-recomp),
  the first (Linux) port of this game, whose findings this port reuses.
- TRG launcher and project structure shared with Earthworm Jim HD and Outpost Kaloki X ports.

South Park: Let's Go Tower Defense Play! is a game by Doublesix and South Park Digital Studios. This project is not
affiliated with or endorsed by them, Comedy Central or Microsoft.
