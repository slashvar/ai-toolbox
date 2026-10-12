# pr-restack

A Claude Code skill that **re-aligns a chain of stacked DRAFT pull requests**. It
chain-rebases each branch onto its parent (the bottom onto the default branch),
build-checks each branch, then pushes with `--force-with-lease`, so child PRs
show only their own commits.

It is the rebase counterpart to `/pr-sync` and the two are mutually exclusive:
`/pr-sync` merges (safe for ready PRs, no force-push); `/pr-restack` rewrites
history, so it hard-aborts if any PR in the stack is not a draft.

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

`git` and an authenticated `gh`. Optionally a stack check skill (e.g. `go-check`)
for the pre-push build check; otherwise the skill builds the touched packages itself.

## History

Generalized from a personal skill in June 2026. The `<owner>/…` branch prefix
replaced a hardcoded one; an internal Go-monorepo build check became a
stack-agnostic "build what this branch touches" step; and notes from private
memory files were inlined as the rules they encoded (stash before batch
checkout, cross-stack red CI, take-HEAD including staged files, build each
branch before push).
