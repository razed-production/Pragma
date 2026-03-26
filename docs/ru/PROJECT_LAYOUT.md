# Структура Проекта

## Верхний Уровень

- [Pragma](../../Pragma) - Visual Studio project и дерево исходников движка
- [assets](../../assets) - runtime и editor-facing контент
- [docs](../../docs) - документация
- [scripts](../../scripts) - helper scripts и smoke tests
- [saved](../../saved) - сгенерированные runtime/editor state файлы
- [x64](../../x64) - build output

## Дерево Исходников

- [Pragma/src/main.cpp](../../Pragma/src/main.cpp) - точка входа процесса
- [Pragma/src/Pragma/Core](../../Pragma/src/Pragma/Core) - bootstrap приложения, scene documents, сериализация, runtime/session flow
- [Pragma/src/Pragma/Platform](../../Pragma/src/Pragma/Platform) - Win32 windowing и input
- [Pragma/src/Pragma/Math](../../Pragma/src/Pragma/Math) - math primitives
- [Pragma/src/Pragma/Assets](../../Pragma/src/Pragma/Assets) - asset ids, manifest и loaders
- [Pragma/src/Pragma/RHI](../../Pragma/src/Pragma/RHI) - backend-neutral graphics contracts
- [Pragma/src/Pragma/RHI/DX11](../../Pragma/src/Pragma/RHI/DX11) - DX11 backend
- [Pragma/src/Pragma/Renderer](../../Pragma/src/Pragma/Renderer) - scene runtime, компоненты и render flow
- [Pragma/src/Pragma/Physics](../../Pragma/src/Pragma/Physics) - интеграция Jolt
- [Pragma/src/Pragma/Editor](../../Pragma/src/Pragma/Editor) - editor windows и workflow
- [Pragma/src/Pragma/DebugUI](../../Pragma/src/Pragma/DebugUI) - diagnostics и поддержка ImGui
- [Pragma/src/Pragma/Scripting](../../Pragma/src/Pragma/Scripting) - bootstrap managed host и runtime bridge

## Дерево Assets

- [assets/scenes](../../assets/scenes) - сериализованные сцены
- [assets/prefabs](../../assets/prefabs) - prefab assets
- [assets/materials](../../assets/materials) - material assets
- [assets/models](../../assets/models) - исходники моделей
- [assets/textures](../../assets/textures) - текстуры
- [assets/scripts](../../assets/scripts) - managed script projects и assemblies

## Генерируемое Состояние

- [saved/editor_layout_state.ini](../../saved/editor_layout_state.ini) - состояние видимости editor layout
- [saved/imgui_layout.ini](../../saved/imgui_layout.ini) - состояние layout ImGui
- [saved/managed_probe_runtime.log](../../saved/managed_probe_runtime.log) - runtime trace managed scripting probe

## Важные Границы

- `Core` оркестрирует runtime и editor document flow.
- `Editor` должен работать через document operations, а не через прямые scene hacks.
- `Renderer` должен потреблять `RHI`, а не DX11 напрямую.
- `Physics` должен оставаться отдельным от rendering и document code.
- `Scripting` должно связываться с runtime-типами, не дублируя модель движка.
