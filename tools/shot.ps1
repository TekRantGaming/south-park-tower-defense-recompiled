param([string]$Name = "shot", [string[]]$Keys = @(), [int]$Wait = 1500)
# Dev helper: optionally taps keys in the game window, then saves a 960x540 copy of it to shots/<Name>.png.
$p = Get-Process south_park_td -ErrorAction Stop | Select-Object -First 1
if ($Keys.Count) { & "$PSScriptRoot\send_keys.ps1" -ProcessId $p.Id -Keys $Keys; Start-Sleep -Milliseconds $Wait }
$full = Join-Path $PSScriptRoot "..\shots\$Name-full.png"
& "$PSScriptRoot\snap_window.ps1" -ProcessId $p.Id -OutFile $full | Out-Null
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Image]::FromFile((Resolve-Path $full))
$s = New-Object System.Drawing.Bitmap $b, 960, 540
$s.Save((Join-Path $PSScriptRoot "..\shots\$Name.png")); $b.Dispose(); $s.Dispose()
Remove-Item $full
