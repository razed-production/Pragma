$ErrorActionPreference = 'Stop'

$projectRoot = 'D:\repos\Pragma'
$runtimeRoot = 'C:\Program Files\dotnet\shared\Microsoft.NETCore.App\10.0.0'
$compiler = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$source = Join-Path $projectRoot 'assets\scripts\Pragma.Managed\Bootstrap.cs'
$output = Join-Path $projectRoot 'assets\scripts\Pragma.Managed\Pragma.Managed.dll'

if (!(Test-Path $compiler))
{
    throw "C# compiler not found: $compiler"
}

& $compiler `
    /nologo `
    /noconfig `
    /target:library `
    /nostdlib `
    /out:$output `
    /reference:"$runtimeRoot\System.Private.CoreLib.dll" `
    /reference:"$runtimeRoot\System.Runtime.dll" `
    /reference:"$runtimeRoot\System.Runtime.InteropServices.dll" `
    /reference:"$runtimeRoot\netstandard.dll" `
    $source

if (!(Test-Path $output))
{
    throw "Managed probe assembly was not produced: $output"
}

Get-Item $output | Select-Object FullName, Length, LastWriteTime
