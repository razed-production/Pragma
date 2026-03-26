$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$exePath = Join-Path $repoRoot "x64\\Debug\\Pragma.exe"

if (-not (Test-Path $exePath))
{
    throw "Executable not found: $exePath"
}

$startTime = Get-Date
$process = $null

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

    Start-Sleep -Seconds 5
    Assert-Alive -Stage "Physics scene runtime" -Process $process

    $crashEvents = @(Get-WinEvent -FilterHashtable @{ LogName = 'Application'; StartTime = $startTime } -ErrorAction SilentlyContinue |
        Where-Object { $_.ProviderName -eq 'Application Error' -and $_.Message -like '*Pragma.exe*' })

    if ($crashEvents)
    {
        $latestCrash = $crashEvents | Select-Object -First 1
        throw "Detected Application Error for Pragma.exe after physics smoke test: $($latestCrash.Message)"
    }

    Write-Host "Physics smoke test passed: startup and runtime remained stable."
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
