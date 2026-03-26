# Pragma Architecture

Pragma is a PC-only 3D engine built around a stable core, explicit subsystem boundaries, and an editor-facing data pipeline that can keep growing without rewriting the foundation.

## Guiding Principles

1. `Core` should not know about DX11 objects or backend-specific rendering types.
2. `Renderer` depends on `RHI`, not on a concrete graphics API.
3. DX11 is the current backend, not the public engine architecture.
4. Scene, prefab, material, physics, and scripting workflows are data-driven.
5. Editor tooling should sit on top of runtime/document systems instead of bypassing them.
6. Managed scripting should extend the existing runtime model instead of replacing it.

## Main Layers

### `Core`

Owns application orchestration, scene documents, serialization, runtime rebuilding, logging, and high-level engine flow.

Examples:
- application bootstrap
- scene document/history
- scene and prefab serializers
- smoke-test friendly startup paths

### `Assets`

Owns `AssetId`, manifest resolution, and asset loading for runtime/editor-facing content such as meshes, textures, materials, scenes, prefabs, and managed script projects.

### `Platform`

Owns the Win32 window, fullscreen/windowed transitions, and raw platform input.

### `RHI`

Defines backend-neutral graphics contracts such as devices, swapchains, resources, and command-list style operations.

### `RHI/DX11`

Implements the current rendering backend. DX11 details must stay inside this layer.

### `Renderer`

Owns scene-facing runtime systems:
- entities and hierarchy
- transforms and component access
- render extraction and frame submission
- camera and behaviour systems
- native and managed script components

### `Physics`

Owns the Jolt integration and the bridge between scene components and runtime bodies.

### `Editor`

Owns editor windows and workflows built on top of scene documents:
- hierarchy
- scene tools
- inspector
- material browser
- prefab browser
- physics debug

### `Scripting`

Owns managed hosting and the bridge between native runtime data and managed C# code.

## Runtime Model

The current runtime is centered around:
- `Scene`
- `SceneObject`
- slot-based components
- `EntityHandle`
- `World`
- `SceneDocument`

The current component set includes:
- `Camera`
- `Camera Controller`
- `Mesh Renderer`
- `Directional Light`
- `Rigid Body`
- `Box Collider`
- `Prefab Instance`
- native `Behaviour`
- `Managed Script`

## Editor And Data Flow

The current workflow is:

1. Content is referenced through `AssetId`.
2. `SceneDocument` loads serialized scene data.
3. `SceneRuntimeBuilder` rebuilds the runtime scene.
4. Editor windows operate through document actions.
5. Save, reload, undo, redo, prefab, and material workflows go back through the document layer.

This is a deliberate rule: editor code should not mutate the runtime scene in ad-hoc ways that bypass the document model.

## Physics And Scripting

Physics and scripting are both first-class runtime subsystems:

- Jolt Physics is integrated through dedicated components and a physics system.
- Native scripting is still supported.
- Managed scripting already has:
  - `hostfxr` bootstrap
  - runtimeconfig probing
  - managed script lifecycle
  - `Entity` / `World` / `Transform` access
  - initial `Camera` / `Light` bindings

## What Must Stay Stable

These boundaries matter the most:

1. `Renderer` must stay backend-neutral.
2. `SceneDocument` must remain the editor-facing source of truth.
3. asset references should continue to use `AssetId`.
4. managed scripting should keep extending the same runtime data model.
5. diagnostics and smoke tests should remain part of everyday development.
