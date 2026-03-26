# Обзор Рендера

## Цель

Pragma использует слоистую rendering-архитектуру:

- `Renderer` владеет scene-facing render flow
- `RHI` владеет backend-neutral контрактами
- `RHI/DX11` реализует текущий backend

Это не даёт DX11 превратиться в архитектуру всего движка.

## Текущий Render Path

Текущий vertical slice включает:
- создание Win32-окна
- создание DX11 device/context/swapchain
- обработку resize
- отправку mesh через renderer
- material-driven rendering
- directional lighting
- editor и diagnostic overlays
- CPU и GPU timing instrumentation
- optional physics overlay rendering

## Runtime Rendering Types

- `Mesh`
- `Material`
- `MeshRendererComponent`
- `CameraComponent`
- `LightComponent`
- renderer-side frame и material constants

## Путь Assets

Типичный поток контента:

1. `AssetId`
2. `AssetManifest`
3. asset loader/importer
4. runtime asset data
5. GPU upload или создание runtime object
6. ссылки из scene components

Materials, scenes, prefabs и managed script projects теперь тоже живут в той же asset-driven модели.

## Правила Для Рендера

- не выпускать DX11-типы за пределы [Pragma/src/Pragma/RHI/DX11](../../Pragma/src/Pragma/RHI/DX11)
- расширять описания `RHI`, а не протягивать backend shortcuts через движок
- держать владение ресурсами явным
- добавлять новые pass-ы через renderer systems, а не через ad-hoc код в bootstrap приложения

## Ближайшее Направление

Rendering path намеренно пока остаётся компактным. Приоритет сейчас в том, чтобы держать его стабильным, пока вокруг взрослеют editor, data, physics и scripting системы.
