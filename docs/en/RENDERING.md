# Rendering Overview

## Goal

Pragma uses a layered rendering architecture:

- `Renderer` owns scene-facing render flow
- `RHI` owns backend-neutral contracts
- `RHI/DX11` implements the current backend

This keeps DX11 from becoming the engine architecture.

## Current Render Path

The current vertical slice includes:
- Win32 window creation
- DX11 device/context/swapchain creation
- resize handling
- mesh submission through the renderer
- material-driven rendering
- directional lighting
- editor/diagnostic overlays
- CPU and GPU timing instrumentation
- optional physics overlay rendering

## Runtime Rendering Types

- `Mesh`
- `Material`
- `MeshRendererComponent`
- `CameraComponent`
- `LightComponent`
- renderer-side frame and material constants

## Asset Path

Typical content flow:

1. `AssetId`
2. `AssetManifest`
3. asset loader/importer
4. runtime asset data
5. GPU upload or runtime object creation
6. scene component references

Materials, scenes, prefabs, and managed script projects now all follow the same asset-driven idea.

## Rendering Rules

- do not leak DX11 types outside [Pragma/src/Pragma/RHI/DX11](../../Pragma/src/Pragma/RHI/DX11)
- extend `RHI` descriptions instead of wiring backend shortcuts through the engine
- keep resource ownership explicit
- add new passes through renderer systems, not through ad-hoc application bootstrap code

## Near-Term Direction

The rendering path is intentionally still compact. The current priority is keeping it stable while editor, data, physics, and scripting systems mature around it.
