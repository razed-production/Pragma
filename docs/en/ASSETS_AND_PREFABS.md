# Assets and Prefabs

Pragma already treats content as data instead of hardcoded scene bootstrap logic.

## Asset Model

The engine uses `AssetId` as the stable content reference layer.

Current asset categories include:
- scenes
- prefabs
- materials
- meshes
- textures
- managed script projects

Asset lookup is driven by [assets/manifest.txt](../../assets/manifest.txt).

## Scene Assets

Scenes are serialized files under [assets/scenes](../../assets/scenes).

They currently store:
- object ids
- names
- hierarchy
- transforms
- component data
- script assignments
- prefab instance metadata

## Material Assets

Materials now live as separate assets under [assets/materials](../../assets/materials) instead of being embedded inline inside every scene object.

This allows:
- material reuse
- editor-side material browsing
- material editing and saving

## Prefab Assets

Prefabs live under [assets/prefabs](../../assets/prefabs).

The current prefab workflow already supports:
- instantiate prefab
- save selected subtree as prefab
- track prefab instance source asset
- basic apply/revert flow

## Current Boundaries

Important rules:
- prefer `AssetId` over hardcoded file paths in engine logic
- keep imported or serialized content separate from runtime objects
- editor workflows should resolve through documents and asset systems, not through one-off shortcuts

## Near-Term Direction

The next likely improvements around assets are:
- more asset browser polish
- stronger prefab override tracking
- richer material editing
