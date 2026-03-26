# Using The Editor

В Pragma уже есть полезный ранний editor workflow.

## Типичная Сессия

1. Открыть движок.
2. Выбирать объекты в `Hierarchy`.
3. Редактировать компоненты в `Inspector`.
4. Использовать верхнее меню для save/reload/undo/redo.
5. При необходимости пользоваться `Material Browser` и `Prefab Browser`.

## Основные Зоны Редактора

- `Hierarchy` для выбора объектов и работы с деревом
- `Scene View` для scene tools и контекста выбора
- `Inspector` для редактирования компонентов
- `Material Browser` для назначения и редактирования материалов
- `Prefab Browser` для инстанцирования и authoring prefab
- `Physics Debug` для инспекции runtime physics

## Правила Безопасной Работы

- при тестировании editor path лучше предпочитать save/reload, а не ручное редактирование файлов
- считать `SceneDocument` source of truth
- следить за notifications и diagnostics при проверке новых workflow
