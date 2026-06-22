# commit

A Claude Code skill for **safe git commits**. Its core rule: if the tip commit
already exists on origin, create a new commit; if it hasn't been pushed yet,
amending is fine. It stages specific files (never `git add -A`), follows the
repo's recent commit style with Conventional-Commits types, and runs a quick
sanity check scoped to the affected package after committing.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/commit`. Because it's a symlink,
any edit in this repo is immediately live in Claude Code — no re-install needed.

If `~/.claude/skills/commit` already exists as a regular directory, the
installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/commit
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
commit/
├── README.md        # this file
├── install.sh       # symlinks skill/ into ~/.claude/skills/commit
└── skill/
    └── SKILL.md     # entry point loaded by Claude Code
```

## Requirements

- `git`.
- Optionally a language/repo quality-pipeline skill (e.g. `go-check`) for the
  post-commit sanity check; otherwise it falls back to the language's standard
  test command.

## History

Generalized from a personal commit skill in June 2026. The only org-specific
content was the post-commit sanity check, which hardcoded an Algolia Go check
skill; it was rewritten to prefer any installed repo-specific check skill and
otherwise detect the stack (Go / Node / etc.) at runtime.
