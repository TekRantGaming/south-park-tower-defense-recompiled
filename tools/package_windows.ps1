<#
  Builds the release configuration and packs the Windows download:
  dist\SouthParkTD-<version>-windows-x64.zip with SouthParkTD\ holding the
  program, its two DLLs, README.txt and licenses. No game files: players
  install those from their own package in the launcher.

  Needs sptd\generated\default (setup.ps1 with your package) and sptd\build.bat's tools.

  .\tools\package_windows.ps1 -Version v0.1.0
#>
param([Parameter(Mandatory = $true)][string]$Version)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

cmd /c "`"$root\sptd\build.bat`" sptd-release"
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

$bin = "$root\sptd\out\build\sptd-release"
$stage = "$root\dist\SouthParkTD"
if (Test-Path $stage) { Get-ChildItem $stage -Recurse -File | ForEach-Object { $_.Delete() } }
New-Item -ItemType Directory -Force "$stage\licenses" | Out-Null
Copy-Item "$bin\south_park_td.exe", "$bin\rexgpu-xenos.dll" $stage
# Always the clean rebuild of rexruntime.dll: the official v0.10.0 one is an
# antivirus false positive (C:\TRG-shared\rexruntime-fix\README.md, rexglue/rexglue-sdk#485).
$clean = "$root\tools\rexglue\win-amd64\bin\rexruntime.dll"
if ((Get-FileHash $clean -Algorithm SHA256).Hash -ne 'E87C3555602C41B18579EF8932639F7AFC9324B0CA448F6E7608010D10EDBBC3') {
    throw "$clean is not the clean rebuild (see C:\TRG-shared\rexruntime-fix)"
}
Copy-Item $clean $stage
Copy-Item "$root\tools\licenses\ReXGlue-LICENSE.txt" "$stage\licenses\"
Copy-Item "$root\tools\rexglue\win-amd64\licenses\SDL3\LICENSE.txt" "$stage\licenses\SDL3-LICENSE.txt"

$readme = @"
South Park: Let's Go Tower Defense Play! PC Port $Version
=========================================================

1. Run south_park_td.exe. The launcher opens.
2. On the Play page click "Install from package..." and pick your own
   South Park: Let's Go Tower Defense Play! Xbox Live Arcade package.
3. Press PLAY.

No game files are included. You need your own package, this exact release:

  Title ID      58410931 (Xbox Live Arcade, content type 000D0000)
  Game version  default.xex 0.0.1.2 (2009-09-09), 8,499,200 bytes,
                CRC32 A21E2AF7

The launcher checks both and tells you if your package is a different version.

Settings: south_park_td.toml next to this program.
Saves:    Documents\south_park_td
Problems: https://github.com/TekRantGaming/south-park-tower-defense-recompiled/issues

Requires Windows 10 or 11 (64-bit) and a DirectX 12 graphics card.
South Park: Let's Go Tower Defense Play! is a game by Doublesix and South Park
Digital Studios. This project is not affiliated with or endorsed by them,
Comedy Central or Microsoft.
"@
[IO.File]::WriteAllText("$stage\README.txt", $readme.Replace("`r`n", "`n").Replace("`n", "`r`n"))

$zip = "$root\dist\SouthParkTD-$Version-windows-x64.zip"
if (Test-Path $zip) { [IO.File]::Delete($zip) }
# Windows' tar writes zip entries with forward slashes, which every unzip tool reads.
& "$env:SystemRoot\System32\tar.exe" -a -c -f $zip -C "$root\dist" SouthParkTD
if ($LASTEXITCODE -ne 0) { throw "Could not create $zip" }
Get-Item $zip | Select-Object Name, Length
(Get-FileHash $zip -Algorithm SHA256).Hash
