# pr

A general-purpose Claude Code skill for **creating pull requests**. It adapts to
any repo by discovering the PR template and contributing guidelines at runtime,
enforces a consistent branch-naming convention, runs the repo's quality gates
before committing, writes a tight bullet-point PR body, and creates the PR with
`gh`.

The skill is **language- and org-agnostic**: the branch prefix is derived from
your own handle (and defers to the repo's own convention if it has one), the
quality-gate table dispatches on project markers (`go.mod`, `package.json`,
`Cargo.toml`, `pyproject.toml`, `Makefile`), and the PR body has hard
constraints (one-line bullets, ≤15 rendered lines, no implementation walkthrough)
with a worked BAD→GOOD example.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/pr`. Because it's a symlink, any
edit in this repo is immediately live in Claude Code — no re-install needed.

If `~/.claude/skills/pr` already exists as a regular directory, the installer
refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/pr
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
pr/
├── README.md        # this file
├── install.sh       # symlinks skill/ into ~/.claude/skills/pr
└── skill/
    └── SKILL.md     # entry point loaded by Claude Code
```

## Requirements

- `gh` (GitHub CLI), authenticated.
- `git`.

## History

Generalized from a personal PR skill in June 2026. De-personalized changes: the
hardcoded `marwan/` branch prefix became a configurable `<owner>/…` convention
that defers to the repo's own scheme; the quality-gate table dropped its
Algolia-monorepo and personal-project rows in favor of generic stack detection;
the Algolia-specific Codacy/`godot` suppression section was removed (it now lives
in the `go-check` skill, referenced by a generic pointer); and the worked
PR-body example was rewritten from an internal-tool narrative into a
stack-neutral caching example.
