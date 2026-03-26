# Using The Editor

Pragma already has a useful early editor workflow.

## Typical Session

1. Open the engine.
2. Select objects in `Hierarchy`.
3. Edit components in `Inspector`.
4. Use the menu bar for save/reload/undo/redo.
5. Use `Material Browser` and `Prefab Browser` when needed.

## Main Editor Areas

- `Hierarchy` for object selection and tree operations
- `Scene View` for scene tools and selection context
- `Inspector` for component editing
- `Material Browser` for material assignment and editing
- `Prefab Browser` for prefab instancing and prefab authoring
- `Physics Debug` for runtime physics inspection

## Safety Rules

- prefer save/reload over manual file edits while testing editor paths
- treat `SceneDocument` as the source of truth
- keep an eye on notifications and diagnostics when testing new workflows
