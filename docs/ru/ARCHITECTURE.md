# Архитектура Pragma

Pragma это PC-only 3D-движок, построенный вокруг стабильного ядра, явных границ подсистем и editor-facing data pipeline, который можно развивать дальше без переписывания фундамента.

## Базовые Принципы

1. `Core` не должен знать о DX11-объектах и backend-specific rendering типах.
2. `Renderer` зависит от `RHI`, а не от конкретного graphics API.
3. DX11 это текущий backend, а не публичная архитектура движка.
4. Scene, prefab, material, physics и scripting workflow должны быть data-driven.
5. Editor tooling должно строиться поверх runtime/document систем, а не обходить их.
6. Managed scripting должно расширять существующую runtime-модель, а не заменять её.

## Основные Слои

### `Core`

Владеет orchestration приложения, документами сцены, сериализацией, rebuild runtime-сцены, логированием и high-level flow движка.

Примеры:
- bootstrap приложения
- scene document/history
- сериализаторы scene и prefab
- startup path, удобный для smoke tests

### `Assets`

Владеет `AssetId`, manifest resolution и загрузкой контента для runtime/editor: meshes, textures, materials, scenes, prefabs и managed script projects.

### `Platform`

Владеет Win32-окном, fullscreen/windowed transitions и raw platform input.

### `RHI`

Определяет backend-neutral graphics-контракты: devices, swapchains, resources и command-list style операции.

### `RHI/DX11`

Реализует текущий rendering backend. DX11-детали должны оставаться только в этом слое.

### `Renderer`

Владеет scene-facing runtime-системами:
- entities и hierarchy
- transforms и component access
- render extraction и frame submission
- camera и behaviour systems
- native и managed script components

### `Physics`

Владеет интеграцией Jolt и мостом между scene-компонентами и runtime bodies.

### `Editor`

Владеет editor-окнами и workflow поверх scene document:
- hierarchy
- scene tools
- inspector
- material browser
- prefab browser
- physics debug

### `Scripting`

Владеет managed hosting и bridge между native runtime data и managed C# code.

## Runtime-Модель

Текущий runtime строится вокруг:
- `Scene`
- `SceneObject`
- slot-based components
- `EntityHandle`
- `World`
- `SceneDocument`

Текущий набор компонентов включает:
- `Camera`
- `Camera Controller`
- `Mesh Renderer`
- `Directional Light`
- `Rigid Body`
- `Box Collider`
- `Prefab Instance`
- native `Behaviour`
- `Managed Script`

## Editor И Поток Данных

Текущий workflow такой:

1. Контент ссылается через `AssetId`.
2. `SceneDocument` загружает сериализованные данные сцены.
3. `SceneRuntimeBuilder` собирает runtime scene.
4. Editor-окна работают через document actions.
5. Save, reload, undo, redo, prefab и material workflow идут обратно через document layer.

Это принципиальное правило: editor-код не должен мутировать runtime-сцену обходными ad-hoc путями мимо document model.

## Physics И Scripting

Physics и scripting уже являются полноценными runtime-подсистемами:

- Jolt Physics интегрирован через отдельные компоненты и physics system.
- Native scripting по-прежнему поддерживается.
- Managed scripting уже имеет:
  - `hostfxr` bootstrap
  - probe `runtimeconfig`
  - lifecycle managed script
  - доступ к `Entity` / `World` / `Transform`
  - начальные bindings для `Camera` / `Light`

## Что Особенно Важно Удержать Стабильным

Эти границы сейчас важнее всего:

1. `Renderer` должен оставаться backend-neutral.
2. `SceneDocument` должен оставаться editor-facing source of truth.
3. asset references должны продолжать идти через `AssetId`.
4. managed scripting должно расширять ту же runtime data model.
5. diagnostics и smoke tests должны оставаться частью обычной разработки.
