param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$OutFile,
    # Optional clicks first, as "x,y" in client pixels (posted to the window: no focus change).
    [string[]]$Clicks = @(),
    [int]$ClickWaitMs = 1500
)
# Dev helper: captures a process's own window with PrintWindow, so the window never
# has to come forward and nothing else on the desktop is captured. Saves JPEG or PNG
# by extension.
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Cap {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
'@
[Cap]::SetProcessDPIAware() | Out-Null
$h = (Get-Process -Id $ProcessId).MainWindowHandle
if ($h -eq [IntPtr]::Zero) { throw "Process $ProcessId has no main window" }
foreach ($c in $Clicks) {
    $x, $y = $c.Split(',') | ForEach-Object { [int]$_ }
    $lp = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
    [Cap]::PostMessage($h, 0x0200, [IntPtr]0, $lp) | Out-Null  # WM_MOUSEMOVE
    Start-Sleep -Milliseconds 100
    [Cap]::PostMessage($h, 0x0201, [IntPtr]1, $lp) | Out-Null  # WM_LBUTTONDOWN
    Start-Sleep -Milliseconds 80
    [Cap]::PostMessage($h, 0x0202, [IntPtr]0, $lp) | Out-Null  # WM_LBUTTONUP
    Start-Sleep -Milliseconds $ClickWaitMs
}
$r = New-Object Cap+RECT
[Cap]::GetClientRect($h, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
[Cap]::PrintWindow($h, $dc, 3) | Out-Null  # PW_CLIENTONLY | PW_RENDERFULLCONTENT
$g.ReleaseHdc($dc)
$format = if ($OutFile -like '*.jpg') { [System.Drawing.Imaging.ImageFormat]::Jpeg } else { [System.Drawing.Imaging.ImageFormat]::Png }
$bmp.Save($OutFile, $format)
$g.Dispose(); $bmp.Dispose()
"Saved $OutFile"
