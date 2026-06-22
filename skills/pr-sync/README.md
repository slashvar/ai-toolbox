# pr-sync

A Claude Code skill for **keeping a PR branch up to date with the default
branch** using merge — never rebase — so it never force-pushes a branch that
reviewers or CI have already seen. It guards against syncing a branch whose PR
has already merged/closed, surfaces failing CI before merging, and finishes with
a clean PR status summary.

It is the merge counterpart to `/pr-restack`. Use `/pr-sync` for ready PRs and
single PRs against main; use `/pr-restack` only for a chain of draft PRs.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/pr-sync`. Because it's a symlink,
any edit in this repo is immediately live in Claude Code — no re-install needed.

If `~/.claude/skills/pr-sync` already exists as a regular directory, the
installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/pr-sync
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
pr-sync/
├── README.md        # this file
├── install.sh       # symlinks skill/ into ~/.claude/skills/pr-sync
└── skill/
    └── SKILL.md     # entry point loaded by Claude Code
```

## Requirements

- `gh` (GitHub CLI), authenticated.
- `git`.

## History

Imported from a personal pr-sync skill in June 2026 essentially unchanged — it
was already stack- and org-neutral (pure git + `gh` against the repo's default
branch, no hardcoded paths or org references).
