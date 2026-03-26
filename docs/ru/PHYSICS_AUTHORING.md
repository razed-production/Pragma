# Physics Authoring

В Pragma уже есть рабочий physics authoring path на базе Jolt Physics.

## Текущие Компоненты

- `RigidBodyComponent`
- `BoxColliderComponent`

## Текущий Workflow

Physics уже можно настраивать через editor:
- добавлять и удалять body и collider components
- смотреть состояние body
- смотреть размер collider
- сохранять и загружать physics-данные через сериализацию сцены
- смотреть physics-данные в отдельном окне `Physics Debug`
- включать physics overlay в сцене

## Поддерживаемые Идеи

Текущая реализация уже включает:
- static и dynamic motion types
- выбор collision layer
- validation warnings для неполных setup-ов
- создание runtime bodies из scene-данных

## Валидация

Редактор уже предупреждает о:
- rigid body без collider
- collider без rigid body
- невалидных collider extents
- runtime bodies, которые не смогли корректно создаться

## Smoke-Покрытие

Для physics runtime уже есть:
- [scripts/smoke/physics_runtime.ps1](../../scripts/smoke/physics_runtime.ps1)

## Текущий Объём

Physics authoring намеренно пока остаётся компактным. Он уже полезен для scene setup и debugging, но это ещё не полный gameplay physics toolset.
