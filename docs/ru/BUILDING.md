# Сборка Pragma

## Требования

- Windows 10 или новее
- Visual Studio 2022
- toolchain `MSVC v143`
- Windows SDK с заголовками и библиотеками Direct3D 11
- окружение `x64`
- установленный `.NET` runtime, если вы хотите запускать sample managed scripting path

## Поддерживаемые Цели

- `Debug|x64`
- `Release|x64`

Не поддерживаются:
- `Win32`
- `x86`
- не-Windows платформы

## Сборка Из Visual Studio

1. Открыть [Pragma.sln](../../Pragma.sln).
2. Выбрать `Debug|x64` или `Release|x64`.
3. Собрать solution.

## Сборка Из Командной Строки

Пример:

```powershell
& 'D:\Programs\VSC\MSBuild\Current\Bin\MSBuild.exe' 'D:\repos\Pragma\Pragma.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m:1
```

Если `MSBuild.exe` установлен в другом месте, нужно подставить путь вашей машины.

## Сборка Managed Sample

Чтобы пересобрать sample managed assembly для C# scripting path:

```powershell
powershell -ExecutionPolicy Bypass -File D:\repos\Pragma\scripts\build_managed_probe.ps1
```

## Выходные Файлы

Debug-бинарник создаётся по пути:

- [x64/Debug/Pragma.exe](../../x64/Debug/Pragma.exe)

## Замечания По Assets И Runtime

- Исполняемый файл ожидает, что layout репозитория останется целым.
- Assets сейчас разрешаются через [assets/manifest.txt](../../assets/manifest.txt).
- Scene, material, prefab и managed script assets живут внутри [assets](../../assets).

## Smoke Tests

В репозитории есть helper scripts в [scripts/smoke](../../scripts/smoke):
- regression test для fullscreen/resize
- physics runtime smoke test
