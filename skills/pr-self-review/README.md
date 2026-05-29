# pr-self-review

A Claude Code skill that runs a **cold-read self-review** of an opened pull
request: it spawns a subagent that hasn't seen the design conversation, plan
files, or project-scoped memory, captures the findings, triages them in one
bulk round-trip, applies the accepted fixes as a single commit, and offers a
re-review pass.

The point is to catch the assumptions and missing context that *you* won't
catch precisely because you designed the change. The reviewer reacts to the
diff the way someone encountering it for the first time would.

The skill is **language- and org-agnostic**. It auto-detects the project's
stack from a marker file (`go.mod`, `Cargo.toml`, `package.json`,
`pyproject.toml`, `pom.xml`, `Gemfile`, …) and loads a matching review profile
so the reviewer applies the right idioms and skips whatever the project's
linter already enforces. An unrecognized stack falls back to a generic review.

## How it works

1. **Identify the PR** — current branch by default, or `/pr-self-review <pr-number>`. Checks the PR is OPEN, HEAD matches origin, and the working tree is clean.
2. **Detect the stack** — pick the profile from `skill/references/review-profiles.md`.
3. **Cold-read review** — a context-suppressed subagent writes structured findings (severity, file:line, issue, suggested fix).
4. **Bulk triage** — you mark `fix` / `skip` / `discuss` / `edit` in one round-trip.
5. **Batch apply** — accepted fixes applied together; you confirm the diff before commit.
6. **Commit & push** — single `chore: address self-review findings` commit; skipped findings are annotated in the review file as a durable record.
7. **Offer re-review** — defaults to no.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/pr-self-review`. Because it's a
symlink, any edit in this repo is immediately live in Claude Code — no
re-install needed.

If `~/.claude/skills/pr-self-review` already exists as a regular directory, the
installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/pr-self-review
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
pr-self-review/
├── README.md                    # this file
├── install.sh                   # symlinks skill/ into ~/.claude/skills/pr-self-review
└── skill/                       # INSTALLABLE PAYLOAD (mirrors ~/.claude/skills/pr-self-review/)
    ├── SKILL.md                 # entry point loaded by Claude Code
    └── references/
        └── review-profiles.md   # per-language focus + linter-skip addenda
```

## Requirements

- `gh` (GitHub CLI), authenticated.
- `git`.
- A Claude Code agent capable of spawning a general-purpose subagent (for the cold-read pass).

## Adding a language profile

Edit `skill/references/review-profiles.md`: add a section with a **Focus**
block (idioms to check) and a **Skip** block (what the stack's linter already
enforces), then add the marker file to the detection table in `skill/SKILL.md`
Step 2. No code changes needed.

## History

Generalized from a Go/Algolia-specific self-review skill (`~/.claude/skills/pr-self-review/`)
in May 2026. No Algolia-proprietary content was carried over: the cold-read
flow, bulk triage, and batch-apply machinery were already stack-neutral; the
Go reviewer persona, idiom focus list, and `golangci-lint`/Codacy skip list
were extracted into the per-language profiles; and the memory-suppression rule
was rewritten from a personal `project_*.md` file convention into a
tool-agnostic principle (withhold design conversation / plan files /
project-scoped memory; allow generic-convention memory).
