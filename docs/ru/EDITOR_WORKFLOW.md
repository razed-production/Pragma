# Editor Workflow

В Pragma уже есть реальный editor loop, хотя это всё ещё ранний редактор, а не production-ready инструмент.

## Основные Окна

- `Hierarchy`
- `Scene View`
- `Inspector`
- `Material Browser`
- `Prefab Browser`
- `Physics Debug`
- `Notifications`
- `Status`
- diagnostic windows вроде profiler и log console

## Базовый Workflow

1. Запустить движок.
2. Загрузить текущий документ сцены.
3. Выбирать объекты в `Hierarchy`.
4. Смотреть и редактировать компоненты в `Inspector`.
5. Сохранять, перезагружать, откатывать и повторять действия через document layer.

Главное архитектурное правило здесь такое: editor actions должны идти через document model, а не мутировать runtime scene обходными ad-hoc путями.

## Authoring Сцены

Редактор уже умеет:
- создавать объекты через templates
- переименовывать, дублировать и удалять
- управлять hierarchy parenting и reparenting
- добавлять и удалять компоненты
- назначать native и managed scripts
- инстанцировать prefabs и выполнять базовый apply/revert flow

## Сохранение Layout

Состояние видимости окон и layout хранится в:
- [saved/editor_layout_state.ini](../../saved/editor_layout_state.ini)
- [saved/imgui_layout.ini](../../saved/imgui_layout.ini)

Это позволяет редактору восстанавливать предыдущий layout вместо старта с нуля при каждом запуске.

## Текущие Ограничения

Редактор уже полезен, но в некоторых местах ещё ранний:
- `Scene View` сейчас больше tool-oriented, чем полноценный dedicated viewport/editor camera workflow
- authoring polish ещё продолжается
- managed script authoring уже работает, но ещё не полностью отполирован
- editor UI со временем ещё можно чище разделить по ролям

## Рекомендации Для Изменений

Когда вы меняете editor code:
- предпочитайте document actions вместо прямых scene hacks
- сохраняйте консистентность save/reload/undo
- по возможности не тащите editor-specific логику в renderer и platform code
