$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Windows.Forms

$user32Source = @"
using System;
using System.Runtime.InteropServices;

public static class PragmaSmokeUser32
{
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetWindowPos(
        IntPtr hWnd,
        IntPtr hWndInsertAfter,
        int X,
        int Y,
        int cx,
        int cy,
        uint uFlags);
}
"@

Add-Type -TypeDefinition $user32Source

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$exePath = Join-Path $repoRoot "x64\\Debug\\Pragma.exe"

if (-not (Test-Path $exePath))
{
    throw "Executable not found: $exePath"
}

$startTime = Get-Date
$process = $null
$wshell = New-Object -ComObject WScript.Shell
$setWindowFlags = 0x0040 # SWP_SHOWWINDOW

function Assert-Alive {
    param(
        [string]$Stage,
        [System.Diagnostics.Process]$Process
    )

    $Process.Refresh()
    if ($Process.HasExited)
    {
        throw "$Stage failed: Pragma.exe exited unexpectedly."
    }

    if (-not $Process.Responding)
    {
        throw "$Stage failed: Pragma.exe is not responding."
    }

    if ($Process.MainWindowHandle -eq 0)
    {
        throw "$Stage failed: Pragma.exe has no main window handle."
    }

    Write-Host "$Stage OK: pid=$($Process.Id) hwnd=$($Process.MainWindowHandle)"
}

try
{
    $process = Start-Process -FilePath $exePath -PassThru
    Start-Sleep -Seconds 3
    Assert-Alive -Stage "Launch" -Process $process

    $null = $wshell.AppActivate($process.Id)
    Start-Sleep -Milliseconds 400
    [System.Windows.Forms.SendKeys]::SendWait('{F11}')
    Start-Sleep -Seconds 4
    Assert-Alive -Stage "Enter fullscreen" -Process $process

    $null = $wshell.AppActivate($process.Id)
    Start-Sleep -Milliseconds 400
    [System.Windows.Forms.SendKeys]::SendWait('{F11}')
    Start-Sleep -Seconds 4
    Assert-Alive -Stage "Return windowed" -Process $process

    [void][PragmaSmokeUser32]::SetWindowPos(
        [IntPtr]$process.MainWindowHandle,
        [IntPtr]::Zero,
        80,
        80,
        1400,
        820,
        $setWindowFlags)
    Start-Sleep -Seconds 2
    Assert-Alive -Stage "Resize 1400x820" -Process $process

    [void][PragmaSmokeUser32]::SetWindowPos(
        [IntPtr]$process.MainWindowHandle,
        [IntPtr]::Zero,
        120,
        120,
        1600,
        900,
        $setWindowFlags)
    Start-Sleep -Seconds 2
    Assert-Alive -Stage "Resize 1600x900" -Process $process

    $crashEvents = @(Get-WinEvent -FilterHashtable @{ LogName = 'Application'; StartTime = $startTime } -ErrorAction SilentlyContinue |
        Where-Object { $_.ProviderName -eq 'Application Error' -and $_.Message -like '*Pragma.exe*' })

    if ($crashEvents)
    {
        $latestCrash = $crashEvents | Select-Object -First 1
        throw "Detected Application Error for Pragma.exe after smoke test: $($latestCrash.Message)"
    }

    Write-Host "Smoke test passed: fullscreen toggle and window resize remained stable."
    exit 0
}
catch
{
    Write-Error $_
    exit 1
}
finally
{
    if ($process -ne $null)
    {
        $process.Refresh()
        if (-not $process.HasExited)
        {
            Stop-Process -Id $process.Id -Force
        }
    }
}
