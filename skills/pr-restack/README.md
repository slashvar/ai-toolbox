# pr-restack

A Claude Code skill for **re-aligning a chain of stacked DRAFT pull requests**.
It chain-rebases each branch onto its parent (the bottom onto the default
branch), build-checks each branch, then `--force-with-lease` pushes — so the
GitHub-rendered history of child PRs stays in sync with their parents.

It is the rebase counterpart to `/pr-sync` and the two are mutually exclusive:
`/pr-sync` merges (safe for ready PRs, no force-push); `/pr-restack` rebases and
force-pushes, which is only safe when **every** PR in the chain is a draft. The
skill hard-aborts if any PR in the stack is non-draft.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/pr-restack`. Because it's a
symlink, any edit in this repo is immediately live in Claude Code — no
re-install needed.

If `~/.claude/skills/pr-restack` already exists as a regular directory, the
installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/pr-restack
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
pr-restack/
├── README.md        # this file
├── install.sh       # symlinks skill/ into ~/.claude/skills/pr-restack
└── skill/
    └── SKILL.md     # entry point loaded by Claude Code
```

## Requirements

- `gh` (GitHub CLI), authenticated.
- `git`.
- Optionally a language/repo build-check skill (e.g. `go-check`) for the
  pre-push build verification; otherwise it builds the touched packages directly.

## History

Generalized from a personal pr-restack skill in June 2026. De-personalized
changes: the `marwan/` branch prefix in examples became `<owner>/…`; the Go /
Algolia-monorepo build check (`~/go/src/github/algolia`, `/alg-go-check`) became
a stack-agnostic "build what this branch touches" step; and the citations to
private feedback-memory files were inlined as the lessons they encoded
(stash-before-batch-checkout, cross-stack red CI, take-HEAD-including-staged,
build-each-branch-before-push).
