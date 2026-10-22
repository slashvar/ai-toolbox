# pr-restack

A Claude Code skill that **re-aligns a chain of stacked pull requests**. It updates
each branch onto its parent (the bottom onto the default branch), build-checks
each branch, then pushes, so child PRs show only their own commits.

The strategy is per branch: drafts are rebased and pushed with
`--force-with-lease`; ready-for-review PRs are merged as `/pr-sync` does, never
force-pushed. Ready PRs must sit below the drafts. In a repo whose
`CONTRIBUTING.md` requires GitHub Stacked PRs, it drives the stack with
`gh stack` (`--no-trunk` rebases only, no `submit`).

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/pr-restack`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/pr-restack` (removes only
the link).

## Layout

```text
pr-restack/
├── README.md
├── install.sh       # symlinks skill/ into ~/.claude/skills/pr-restack
└── skill/
    └── SKILL.md
```

## Requirements

`git` and an authenticated `gh`; the `gh stack` extension and its skill for repos
that require GitHub Stacked PRs. Optionally a stack check skill (e.g. `go-check`)
for the pre-push build check; otherwise the skill builds the touched packages itself.

## History

Generalized from a personal skill in June 2026. The `<owner>/…` branch prefix
replaced a hardcoded one; an internal Go-monorepo build check became a
stack-agnostic "build what this branch touches" step; and notes from private
memory files were inlined as the rules they encoded (stash before batch
checkout, cross-stack red CI, take-HEAD including staged files, build each
branch before push).
