# Building Pragma

## Requirements

- Windows 10 or newer
- Visual Studio 2022
- MSVC v143 toolchain
- Windows SDK with Direct3D 11 headers and libraries
- `x64` environment
- `.NET` runtime installed if you want to run the managed scripting sample

## Supported Targets

- `Debug|x64`
- `Release|x64`

Unsupported:
- `Win32`
- `x86`
- non-Windows platforms

## Build From Visual Studio

1. Open [Pragma.sln](../../Pragma.sln).
2. Select `Debug|x64` or `Release|x64`.
3. Build the solution.

## Build From Command Line

Example:

```powershell
& 'D:\Programs\VSC\MSBuild\Current\Bin\MSBuild.exe' 'D:\repos\Pragma\Pragma.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m:1
```

Adjust the `MSBuild.exe` path for your machine if needed.

## Managed Sample Build

To rebuild the sample managed assembly used by the C# scripting path:

```powershell
powershell -ExecutionPolicy Bypass -File D:\repos\Pragma\scripts\build_managed_probe.ps1
```

## Output

The debug executable is written to:

- [x64/Debug/Pragma.exe](../../x64/Debug/Pragma.exe)

## Assets And Runtime Notes

- The executable expects the repository layout to stay intact.
- Assets are currently resolved through [assets/manifest.txt](../../assets/manifest.txt).
- Scene, material, prefab, and managed script assets all live under [assets](../../assets).

## Smoke Tests

The repository includes helper scripts under [scripts/smoke](../../scripts/smoke):
- fullscreen/resize regression test
- physics runtime smoke test
