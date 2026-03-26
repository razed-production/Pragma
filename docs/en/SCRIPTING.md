# Scripting

Pragma currently supports two scripting paths:
- native scripting
- early managed C# scripting

## Native Scripting

The native path is built around:
- `BehaviourComponent`
- `ScriptableEntity`
- `NativeScriptRegistry`
- `World`
- `EntityHandle`

This path is still useful and also serves as a reference model for the managed side.

## Managed C# Scripting

The managed path already includes:
- `hostfxr` bootstrap
- runtimeconfig probing
- assembly entry-point resolution
- managed script lifecycle
- managed script assignment through scene/editor paths

## Current Managed API Surface

Managed code already has access to:
- time snapshot
- logging callback
- entity lookup
- validity checks
- entity name
- parent/children queries
- active camera entity
- entity count
- transform read/write
- initial camera/light read/write bindings

## Current Limitations

The managed path is already real, but still early:
- hot reload is not finished
- diagnostics can grow further
- the managed gameplay API is still expanding
- editor authoring polish can improve further

## Trace And Diagnostics

Managed bootstrap and lifecycle traces are written to:
- [saved/managed_probe_runtime.log](../../saved/managed_probe_runtime.log)

## Recommended Direction

The current direction is to keep native and managed scripting aligned around the same runtime world/entity model so that C# becomes an extension of the engine, not a parallel architecture.
