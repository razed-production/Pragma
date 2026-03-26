# Scripting

В Pragma сейчас есть два scripting path:
- native scripting
- ранний managed C# scripting

## Native Scripting

Native path строится вокруг:
- `BehaviourComponent`
- `ScriptableEntity`
- `NativeScriptRegistry`
- `World`
- `EntityHandle`

Этот путь всё ещё полезен сам по себе и одновременно служит эталонной моделью для managed-стороны.

## Managed C# Scripting

Managed path уже включает:
- `hostfxr` bootstrap
- probe `runtimeconfig`
- resolution entry point из assembly
- lifecycle managed script
- назначение managed script через scene/editor path

## Текущий Managed API Surface

Managed code уже имеет доступ к:
- time snapshot
- logging callback
- entity lookup
- validity checks
- имени entity
- parent/children queries
- active camera entity
- entity count
- чтению и записи transform
- начальным read/write bindings для camera/light

## Текущие Ограничения

Managed path уже настоящий, но пока ранний:
- hot reload ещё не завершён
- diagnostics ещё можно усиливать
- managed gameplay API ещё расширяется
- editor authoring polish ещё можно улучшать

## Trace И Diagnostics

Трассировка bootstrap и lifecycle managed scripting пишется в:
- [saved/managed_probe_runtime.log](../../saved/managed_probe_runtime.log)

## Рекомендуемое Направление

Текущая стратегия такая: native и managed scripting должны опираться на одну и ту же runtime world/entity model, чтобы C# был расширением движка, а не параллельной архитектурой.
