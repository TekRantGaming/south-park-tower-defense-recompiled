param([string[]]$Extra = @(), [int]$BootWait = 35, [string]$Preset = "sptd-relwithdebinfo")
# Dev helper: (re)starts the dev build with a fresh run.log and waits for the title screen.
Get-Process south_park_td -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
$d = Join-Path $PSScriptRoot "..\sptd\out\build\$Preset" | Resolve-Path
Remove-Item "$d\run.log" -ErrorAction SilentlyContinue
$args = @("--game_data_root=`"$(Resolve-Path (Join-Path $PSScriptRoot '..\game'))`"", '--log_file=run.log', '--license_mask=1') + $Extra
Start-Process -FilePath "$d\south_park_td.exe" -ArgumentList $args -WorkingDirectory $d | Out-Null
Start-Sleep $BootWait
