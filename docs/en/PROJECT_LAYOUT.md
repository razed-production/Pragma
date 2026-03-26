# Project Layout

## Top Level

- [Pragma](../../Pragma) - Visual Studio project and engine source tree
- [assets](../../assets) - runtime and editor-facing content
- [docs](../../docs) - documentation
- [scripts](../../scripts) - helper and smoke-test scripts
- [saved](../../saved) - generated runtime/editor state files
- [x64](../../x64) - build output

## Source Tree

- [Pragma/src/main.cpp](../../Pragma/src/main.cpp) - process entry point
- [Pragma/src/Pragma/Core](../../Pragma/src/Pragma/Core) - application bootstrap, scene documents, serialization, runtime/session flow
- [Pragma/src/Pragma/Platform](../../Pragma/src/Pragma/Platform) - Win32 windowing and input
- [Pragma/src/Pragma/Math](../../Pragma/src/Pragma/Math) - math primitives
- [Pragma/src/Pragma/Assets](../../Pragma/src/Pragma/Assets) - asset ids, manifest, and loaders
- [Pragma/src/Pragma/RHI](../../Pragma/src/Pragma/RHI) - backend-neutral graphics contracts
- [Pragma/src/Pragma/RHI/DX11](../../Pragma/src/Pragma/RHI/DX11) - DX11 backend
- [Pragma/src/Pragma/Renderer](../../Pragma/src/Pragma/Renderer) - scene runtime, components, render flow
- [Pragma/src/Pragma/Physics](../../Pragma/src/Pragma/Physics) - Jolt integration
- [Pragma/src/Pragma/Editor](../../Pragma/src/Pragma/Editor) - editor windows and workflows
- [Pragma/src/Pragma/DebugUI](../../Pragma/src/Pragma/DebugUI) - diagnostics and ImGui support
- [Pragma/src/Pragma/Scripting](../../Pragma/src/Pragma/Scripting) - managed host bootstrap and runtime bridge

## Asset Tree

- [assets/scenes](../../assets/scenes) - serialized scenes
- [assets/prefabs](../../assets/prefabs) - prefab assets
- [assets/materials](../../assets/materials) - material assets
- [assets/models](../../assets/models) - model sources
- [assets/textures](../../assets/textures) - textures
- [assets/scripts](../../assets/scripts) - managed script projects and assemblies

## Generated State

- [saved/editor_layout_state.ini](../../saved/editor_layout_state.ini) - editor layout visibility state
- [saved/imgui_layout.ini](../../saved/imgui_layout.ini) - ImGui layout state
- [saved/managed_probe_runtime.log](../../saved/managed_probe_runtime.log) - managed scripting runtime probe trace

## Key Boundaries

- `Core` orchestrates runtime and editor document flow.
- `Editor` should act through document operations, not direct scene hacks.
- `Renderer` consumes `RHI`, not DX11 directly.
- `Physics` stays separate from rendering and document code.
- `Scripting` should bridge to runtime types without duplicating the engine model.
