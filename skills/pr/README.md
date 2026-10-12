# pr

A general-purpose Claude Code skill for **creating pull requests**. It discovers
the repo's PR template and contributing guide at runtime, names the branch by
convention, runs the repo's quality gates, writes a tight bullet-point body, and
opens the PR with `gh`.

It is language- and org-agnostic: the branch prefix comes from your own handle
(deferring to the repo's convention), the quality-gate table dispatches on project
markers (`go.mod`, `package.json`, `Cargo.toml`, `pyproject.toml`, `Makefile`),
and the body has hard constraints (one-line bullets, at most 15 rendered lines, no
implementation walkthrough) with a worked BAD→GOOD example.

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/pr`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/pr` (removes only
the link).

## Layout

```text
pr/
├── README.md
├── install.sh       # symlinks skill/ into ~/.claude/skills/pr
└── skill/
    └── SKILL.md
```

## Requirements

`git` and an authenticated `gh`.

## History

Generalized from a personal skill in June 2026: the hardcoded branch prefix became
a configurable `<owner>/…` convention that defers to the repo's own; the
quality-gate table became generic stack detection; scanner-suppression guidance
moved to `go-check`, referenced by a generic pointer; and the worked PR-body
example became a stack-neutral caching example.
