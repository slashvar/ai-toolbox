# corner-cases

A Claude Code skill that identifies and eliminates **corner case programming** — an anti-pattern where code is split into too many specific sub-cases that the general case already handles.

The skill triggers when reviewing, simplifying, or refactoring code; during TDD green/refactor steps; and whenever boundary guards, defensive "should-not-happen" returns, or near-duplicate functions appear. It operates under three principles:

- **A.** Write the general case, not the specific one.
- **B.** Use what you know about state (contracts, postconditions, invariants).
- **C.** Tell apart true redundancy from essential variation (do-not-touch patterns).

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/corner-cases`. Because it's a symlink, any edit in this repo is immediately live in Claude Code — no re-install needed.

If `~/.claude/skills/corner-cases` already exists as a regular directory, the installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/corner-cases
```

Safe because the install is a symlink — only the link is removed, the repo is untouched.

## Layout

```
corner-cases/
├── skill/                       # INSTALLABLE PAYLOAD (mirrors ~/.claude/skills/corner-cases/)
│   ├── SKILL.md                 # entry point loaded by Claude Code
│   ├── references/
│   │   ├── patterns.md          # 8-pattern catalog under 3 principles
│   │   └── examples.md          # worked examples from the research exercises
│   └── evals/
│       └── evals.json           # runnable via skill-creator
└── research/                    # work that produced the skill
    ├── CLAUDE.md                # per-project coding guidelines (scoped to this project)
    ├── PATTERNS.md              # long-form pattern catalog
    ├── LEARNINGS.md             # observations from each exercise
    └── exercises/               # example code: list, btree, spanner, etc.
```

## Running evals

The eval bundled in `skill/evals/evals.json` uses `skill-creator`:

```
/skill-creator eval corner-cases
```

It runs the `list-refactor` exercise twice — with and without the skill — and grades the outputs. Paths in `evals.json` point inside this repo.

## History

Consolidated from `~/code/corner-case-programming/` (research) and `~/.claude/skills/corner-cases/` (installed skill) in April 2026. Algolia-proprietary source files (`queryparam-parse`, `queryparam-proto`) were excluded; only the ANALYSIS.md writeups remain.
