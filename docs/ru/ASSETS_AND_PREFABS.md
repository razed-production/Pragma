# Assets и Prefabs

Pragma уже относится к контенту как к данным, а не как к захардкоженному scene bootstrap logic.

## Asset-Модель

Движок использует `AssetId` как стабильный слой ссылок на контент.

Текущие категории assets включают:
- scenes
- prefabs
- materials
- meshes
- textures
- managed script projects

Поиск assets идёт через [assets/manifest.txt](../../assets/manifest.txt).

## Scene Assets

Сцены сериализуются в файлы внутри [assets/scenes](../../assets/scenes).

Сейчас они хранят:
- object ids
- names
- hierarchy
- transforms
- component data
- script assignments
- prefab instance metadata

## Material Assets

Материалы теперь живут как отдельные assets в [assets/materials](../../assets/materials), а не как inline-данные внутри каждого scene object.

Это даёт:
- повторное использование материалов
- editor-side material browsing
- редактирование и сохранение материалов

## Prefab Assets

Prefabs лежат в [assets/prefabs](../../assets/prefabs).

Текущий prefab workflow уже поддерживает:
- инстанцирование prefab
- сохранение выбранного поддерева как prefab
- хранение source asset у prefab instance
- базовый apply/revert flow

## Текущие Границы

Важные правила:
- предпочитать `AssetId`, а не захардкоженные пути в engine logic
- держать imported или serialized content отдельно от runtime objects
- editor workflow должен идти через documents и asset systems, а не через одноразовые shortcuts

## Ближайшее Направление

Следующие вероятные улучшения вокруг assets:
- более зрелый asset browser
- более сильный учёт prefab overrides
- более богатое редактирование материалов
