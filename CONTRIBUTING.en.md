# Contributing to Pragma

Pragma is intentionally built around a clean long-lived foundation. Contributions are welcome, but architectural discipline matters more here than short-lived feature experiments.

## Core Rules

- Windows desktop only
- `x64` only
- 3D only
- `C++20`
- public engine modules must remain graphics-API neutral
- `Renderer` must not depend on `ID3D11*`
- DX11 code must stay inside `RHI/DX11`

## Before Changing Code

1. Read [docs/ARCHITECTURE.md](./docs/ARCHITECTURE.md).
2. Review [docs/PROJECT_LAYOUT.md](./docs/PROJECT_LAYOUT.md).
3. If the change touches rendering contracts, read [docs/RENDERING.md](./docs/RENDERING.md).

## Contribution Expectations

- prefer small, reviewable changes
- keep ownership and lifetime rules explicit
- do not leak backend-specific assumptions into shared headers
- document the purpose and boundaries of every new subsystem
- update documentation in the same change when setup, architecture, or workflows move

## Rendering Rules

- renderer features must go through `Renderer -> RHI`, not directly to the backend
- `RHI` contracts should stay compatible with a future explicit API model
- DX11 should remain an implementation detail and compatibility backend

## Asset Rules

- prefer `AssetId` references over hardcoded file paths in engine code
- imported asset data and runtime GPU resources must stay separate
- document new asset formats and their assumptions

## Minimum Verification

- `Debug|x64` builds successfully
- the application launches
- resize still works
- camera/input still behave correctly
- the scene still renders through the abstract path

## Documentation

If a change affects onboarding, architecture, build steps, runtime flow, editor workflow, or extension points, update the matching documentation under [docs](./docs).
