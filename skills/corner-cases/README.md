# corner-cases

A Claude Code skill that identifies and eliminates **corner case programming** — an anti-pattern where code is split into too many specific sub-cases that the general case already handles.

The skill triggers when reviewing, simplifying, or refactoring code; during TDD green/refactor steps; and whenever boundary guards, defensive "should-not-happen" returns, or near-duplicate functions appear. It operates under three principles:

- **A.** Write the general case, not the specific one.
- **B.** Use what you know about state (contracts, postconditions, invariants).
- **C.** Tell apart true redundancy from essential variation (do-not-touch patterns).

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/corner-cases`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/corner-cases` (removes only
the link).

## Layout

```text
corner-cases/
├── skill/                       # INSTALLABLE PAYLOAD (mirrors ~/.claude/skills/corner-cases/)
│   ├── SKILL.md                 # entry point loaded by Claude Code
│   ├── references/
│   │   ├── patterns.md          # pattern catalog under 3 principles
│   │   └── examples.md          # before/after snippets from exercises and real refactors
│   └── evals/
│       └── evals.json           # runnable via skill-creator
└── research/                    # work that produced the skill
    ├── CLAUDE.md                # per-project coding guidelines (scoped to this project)
    ├── PATTERNS.md              # long-form pattern catalog
    ├── LEARNINGS.md             # observations from each exercise
    └── exercises/               # example code: list, list-tdd, btree, spanner
```

## Running evals

The eval bundled in `skill/evals/evals.json` uses `skill-creator`:

```text
/skill-creator eval corner-cases
```

It runs the `list-refactor` exercise twice — with and without the skill — and grades the outputs. Paths in `evals.json` are relative to the repo root, so run it from there.

## History

Consolidated from `~/code/corner-case-programming/` (research) and `~/.claude/skills/corner-cases/` (installed skill) in April 2026. Exercises on private source were dropped; only their lessons remain in `research/LEARNINGS.md`.
