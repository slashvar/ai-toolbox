# go-check

A Claude Code skill that runs the **Go quality pipeline** (lint → test) for any Go
repository. It scopes to the changed packages for fast pre-push checks, gates only
on issues the diff introduces (reporting older findings separately), matches CI's
pinned `golangci-lint` so local runs reproduce CI, handles infra-bound tests and
build-tagged dirs, and knows how to suppress static-analysis false positives that
fire only post-push (Codacy/Semgrep).

## How it works

1. **Setup**: install the `golangci-lint` version pinned in `.github/workflows/`, so rules a stale binary would skip still run.
2. **Quick check**: lint (`--new-from-rev`) and `go test` on the changed packages, not `./...`; fix and re-run on failure.
3. **Full pipeline**: whole-repo lint and `go test -race`, with CI's build tags, for pre-merge sweeps.
4. **Infra-bound tests**: skip with the repo's unit target, `-skip` or `-short`, and say so.
5. **Codacy/Semgrep**: the inline `nosemgrep: <rule-id>` form that satisfies a `godot`-enabled lint, common rule ids, and how to find new ones.

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/go-check`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/go-check` (removes only
the link).

## Layout

```text
go-check/
├── README.md
├── install.sh       # symlinks skill/ into ~/.claude/skills/go-check
└── skill/
    ├── SKILL.md
    └── evals/evals.json   # runnable via skill-creator
```

## Requirements

- Go toolchain and `golangci-lint` (the skill can install the CI-pinned version).
- `gh`, only for the Semgrep rule-id search recipe.

## History

Generalized in June 2026 from a monorepo-specific skill: monorepo paths and
module layout became package-scoped paths and standard `go test` calls, and the
eval prompts lost their private paths. The Codacy/Semgrep section was kept
because those tools and `godot` are general-purpose; only repo names and
verification dates were stripped.
