# Editor Workflow

Pragma already has a real editor loop, even though it is still an early-stage editor rather than a production-ready tool.

## Main Windows

- `Hierarchy`
- `Scene View`
- `Inspector`
- `Material Browser`
- `Prefab Browser`
- `Physics Debug`
- `Notifications`
- `Status`
- diagnostics windows such as profiler and log console

## Core Workflow

1. Open the engine.
2. Load the current scene document.
3. Select objects in `Hierarchy`.
4. Inspect and edit components in `Inspector`.
5. Save, reload, undo, or redo through the document layer.

The key architectural rule is that editor actions should go through the document model instead of directly mutating the runtime scene in ad-hoc ways.

## Scene Authoring

The editor already supports:
- object creation templates
- rename, duplicate, delete
- hierarchy parenting and reparenting
- component add/remove flow
- managed and native script assignment
- prefab instancing and basic prefab apply/revert flow

## Layout Persistence

Editor window visibility and layout state are persisted under:
- [saved/editor_layout_state.ini](../../saved/editor_layout_state.ini)
- [saved/imgui_layout.ini](../../saved/imgui_layout.ini)

This allows the editor to restore the last layout instead of starting from scratch every run.

## Current Limitations

The editor is already useful, but still early in a few places:
- scene view is tool-oriented rather than a full dedicated viewport/editor camera workflow
- authoring polish is still improving
- managed script authoring is functional but not yet fully polished
- there is still room to separate editor UI concerns more cleanly over time

## Recommended Contributor Mindset

When changing editor code:
- prefer document actions over direct scene hacks
- preserve save/reload/undo consistency
- keep editor-specific logic out of renderer and platform code when possible
