# pr-self-review

A Claude Code skill that runs a **cold-read self-review** of an opened pull
request: it spawns a subagent that hasn't seen the design conversation, plan
files, or project-scoped memory, captures the findings, triages them in one
bulk round-trip, applies the accepted fixes as a single commit, and offers a
re-review pass.

The point is to catch the assumptions and missing context that *you* won't
catch precisely because you designed the change. The reviewer reacts to the
diff the way someone encountering it for the first time would.

The skill detects the project's stack from a marker file (`go.mod`,
`Cargo.toml`, `package.json`, `pyproject.toml`, `pom.xml`, `Gemfile`, …) and
loads a matching review profile: the reviewer applies the right idioms and
skips what the project's linter already enforces. An unknown stack gets a
generic review.

Findings stay local. In a repo that forbids AI-written PR text, the skill only
proposes wording; it never posts or edits the PR.

## How it works

1. **Identify the PR** — current branch by default, or `/pr-self-review <pr-number>`. Checks the PR is OPEN and HEAD matches origin; stashes a dirty working tree and restores it at the end.
2. **Detect the stack** — pick the profile from `skill/references/review-profiles.md`.
3. **Cold-read review** — a context-suppressed subagent writes structured findings (severity, file:line, issue, suggested fix). Settled decisions go in its brief so it does not re-raise them.
4. **Verify and bulk triage** — each finding is checked against the code and tests first; then you mark `fix` / `skip` / `discuss` / `edit` in one round-trip.
5. **Batch apply** — accepted fixes applied together; you confirm the diff before commit.
6. **Commit & push** — single `chore(<scope>): address self-review findings` commit; skipped findings are annotated in the review file as a durable record.
7. **Offer re-review** — defaults to no.

## Install

`./install.sh` symlinks `skill/` into `~/.claude/skills/pr-self-review`, so edits in this
repo are live immediately. It refuses to overwrite a real directory there:
remove or move it first. Uninstall with `rm ~/.claude/skills/pr-self-review` (removes only
the link).

## Layout

```text
pr-self-review/
├── README.md                    # this file
├── install.sh                   # symlinks skill/ into ~/.claude/skills/pr-self-review
└── skill/                       # installable payload
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
