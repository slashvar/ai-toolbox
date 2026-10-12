# pr-sync

A Claude Code skill that **keeps a PR branch up to date with the default branch**
using merge, never rebase, so it never force-pushes a branch reviewers or CI have
seen. It refuses to sync a PR that already merged or closed, surfaces failing CI
before merging, builds the merge result before pushing (skipping build-tagged Go
dirs), and ends with a PR status summary.

It is the merge counterpart to `/pr-restack`: use `/pr-sync` for ready PRs and
single PRs against main, `/pr-restack` only for a chain of drafts.

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/pr-sync`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/pr-sync` (removes only
the link).

## Layout

```text
pr-sync/
├── README.md
├── install.sh       # symlinks skill/ into ~/.claude/skills/pr-sync
└── skill/
    └── SKILL.md
```

## Requirements

`git` and an authenticated `gh`.

## History

Imported from a personal skill in June 2026, essentially unchanged: it was
already stack- and org-neutral. Later additions: the build-tag-aware Go build
check and the "never pipe the merge" rule.
