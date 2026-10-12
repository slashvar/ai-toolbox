# commit

A Claude Code skill for **safe git commits**. The rule: if the tip commit is
already on origin, make a new commit; if not, amending is fine. It stages named
files (never `git add -A`), follows the repo's commit style with
Conventional-Commits types, and runs a quick sanity check on the affected
packages afterwards.

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/commit`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/commit` (removes only
the link).

## Layout

```text
commit/
├── README.md
├── install.sh       # symlinks skill/ into ~/.claude/skills/commit
└── skill/
    └── SKILL.md
```

## Requirements

`git`. Optionally a stack check skill (e.g. `go-check`) for the post-commit
check; otherwise it runs the language's standard test command.

## History

Generalized from a personal skill in June 2026. The only org-specific content, a
hardcoded internal Go check skill in the post-commit check, became "prefer any
installed check skill, else detect the stack at runtime".
