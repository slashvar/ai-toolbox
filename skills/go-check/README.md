# go-check

A Claude Code skill that runs the **Go quality pipeline** (lint → test) for any
Go repository. It scopes to the packages you actually touched for fast pre-push
checks, matches CI's pinned `golangci-lint` version so local runs reproduce CI,
and knows how to suppress the static-analysis false positives that only fire
post-push (Codacy/Semgrep).

## How it works

1. **Setup** — detect and install the `golangci-lint` version pinned in
   `.github/workflows/` so local lint matches CI (godot/revive rules a stale
   binary would skip).
2. **Quick check** — `golangci-lint run` + `go test` scoped to the changed
   package(s), not `./...`. Fix-and-re-run loop on failure.
3. **Full pipeline** — whole-repo lint + `go test -race` (with integration/nightly
   build tags if CI uses them) for pre-merge sweeps.
4. **Codacy/Semgrep** — the verified inline `nosemgrep: <rule-id>` suppression
   form (the only one that satisfies a `godot`-enabled lint), a table of
   commonly-seen rule ids, and the recipe for finding new ones.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/go-check`. Because it's a symlink,
any edit in this repo is immediately live in Claude Code — no re-install needed.

If `~/.claude/skills/go-check` already exists as a regular directory, the
installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/go-check
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
go-check/
├── README.md            # this file
├── install.sh           # symlinks skill/ into ~/.claude/skills/go-check
└── skill/
    ├── SKILL.md         # entry point loaded by Claude Code
    └── evals/
        └── evals.json   # runnable via skill-creator
```

## Requirements

- Go toolchain (`go`).
- `golangci-lint` (the skill can install the CI-pinned version for you).
- `gh` (GitHub CLI) — only for the GitHub code-search recipe used to look up
  Semgrep rule ids.

## History

Generalized from `algolia-go-check` in June 2026. De-personalized changes: the
`~/go/src/github/algolia/` monorepo paths and module layout
(`./modules/services/<area>/internal/...`, `make test-incremental`) became
generic package-scoped paths and standard `go test` invocations; the eval
prompts dropped their Algolia paths. The Codacy/Semgrep suppression section was
**kept by request** — Codacy, Semgrep, and `godot` are general-purpose tools —
with only the Algolia repo names (`metis`, `bag-of-docs`) and "verified Apr 2026"
provenance stripped out; the rule-id table and the inline-`nosemgrep` mechanics
are unchanged.
