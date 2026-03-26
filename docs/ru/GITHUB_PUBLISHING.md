# Инструкция По Публикации На GitHub

Используйте этот чеклист перед публичной публикацией репозитория или первым push в новый GitHub remote.

## Перед Публикацией

1. проверить [.gitignore](../../.gitignore)
2. проверить [.gitattributes](../../.gitattributes)
3. убедиться, что [README.md](../../README.md) и [docs](../../docs) актуальны
4. выбрать публичную лицензию и добавить файл `LICENSE`
5. удалить machine-specific или временные файлы, если они ещё остались

## Рекомендуемые Проверки

- собрать `Debug|x64`
- запустить приложение
- убедиться, что editor открывается
- проверить, что smoke-test scripts всё ещё соответствуют текущей ветке

## Рекомендуемые Первые Шаги В GitHub

```powershell
git init
git add .
git commit -m "Initial public project snapshot"
git branch -M main
git remote add origin <your-github-repo-url>
git push -u origin main
```

## Что Полезно Добавить После Первого Push

Не обязательно, но очень желательно:
- `LICENSE`
- workflow для release notes
- issue templates
- pull request template
- CI хотя бы для одного smoke build

В этом репозитории уже есть:
- [.gitignore](../../.gitignore)
- [.gitattributes](../../.gitattributes)
- [.editorconfig](../../.editorconfig)
- [issue templates](../../.github/ISSUE_TEMPLATE)
- [pull request template](../../.github/PULL_REQUEST_TEMPLATE.md)

## Важное Замечание

Не публикуйте секреты, локальные credentials или проприетарные third-party файлы, которые нельзя распространять публично.
