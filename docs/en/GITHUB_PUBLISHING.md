# GitHub Publishing Guide

Use this checklist before making the repository public or pushing it to a fresh GitHub remote.

## Before Publishing

1. review [.gitignore](../../.gitignore)
2. review [.gitattributes](../../.gitattributes)
3. confirm [README.md](../../README.md) and [docs](../../docs) are up to date
4. decide on a public license and add a `LICENSE` file
5. remove machine-specific or temporary files if needed

## Recommended Sanity Checks

- build `Debug|x64`
- launch the app
- verify the editor opens
- verify smoke-test scripts still make sense for the current branch

## Suggested First GitHub Steps

```powershell
git init
git add .
git commit -m "Initial public project snapshot"
git branch -M main
git remote add origin <your-github-repo-url>
git push -u origin main
```

## Suggested Repository Additions

Optional but recommended after the first push:
- `LICENSE`
- release notes workflow
- issue templates
- pull request template
- CI for at least one smoke build

This repository already includes:
- [.gitignore](../../.gitignore)
- [.gitattributes](../../.gitattributes)
- [.editorconfig](../../.editorconfig)
- [issue templates](../../.github/ISSUE_TEMPLATE)
- [pull request template](../../.github/PULL_REQUEST_TEMPLATE.md)

## Important Note

Do not publish secrets, local credentials, or proprietary third-party files that you do not have the right to redistribute.
