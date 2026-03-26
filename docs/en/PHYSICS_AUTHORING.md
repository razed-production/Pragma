# Physics Authoring

Pragma already has a working physics authoring path built on Jolt Physics.

## Current Components

- `RigidBodyComponent`
- `BoxColliderComponent`

## Current Workflow

Physics can already be authored through the editor:
- add/remove body and collider components
- inspect body state
- inspect collider size
- save and reload physics data through scene serialization
- view physics data in a dedicated `Physics Debug` window
- enable a physics overlay in the scene

## Supported Concepts

The current implementation includes:
- static and dynamic motion types
- collision layer selection
- validation warnings for incomplete setups
- scene-driven runtime body creation

## Validation

The editor already warns about:
- rigid bodies without colliders
- colliders without rigid bodies
- invalid collider extents
- runtime bodies that failed to materialize correctly

## Smoke Coverage

Physics runtime coverage includes:
- [scripts/smoke/physics_runtime.ps1](../../scripts/smoke/physics_runtime.ps1)

## Current Scope

Physics authoring is intentionally still compact. It is already useful for scene setup and debugging, but it is not yet a full gameplay physics toolset.
