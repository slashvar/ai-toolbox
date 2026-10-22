---
name: pr
description: >
  General-purpose PR creation skill that enforces a consistent branch naming
  convention (<owner>/<type>/<scope>/<short-description>), discovers and uses the
  repo's PR template (omitting empty sections), checks CONTRIBUTING.md, uses
  conventional commits, and writes bullet-point descriptions. Use this skill
  whenever the user wants to create a pull request — triggers on "create a PR",
  "open a PR", "make a PR", "let's PR this", "ship it", "/pr", or any request to
  push changes and open a pull request. Do NOT trigger for plain commits without
  PR intent — use /commit for those.
---

# pr

Open a pull request. The skill discovers the repo's PR template and contributing
guide at runtime, so it fits any repo.

## Step 1 — Understand the changes

```bash
git status
git diff HEAD
git log --oneline -10               # recent commit style
git fetch --quiet origin
gh repo view --json defaultBranchRef --jq .defaultBranchRef.name
git log --oneline origin/<default-branch>..HEAD   # commits this branch adds
```

The last list must hold *only* this change's commits. With several terminals or
sessions open, `HEAD` can shift silently, so a branch cut "off main" may carry
another feature's commits and an inflated diff. If unrelated commits appear, stop
and tell the user. The usual fix is to re-create the branch from the intended
base and re-apply the change (cherry-pick, or `rebase --onto`).

**Intentional stacked PR**: when the branch is meant to stack on a parent branch
(Step 6), the parent's commits are expected. Confirm the extras are exactly the
parent's.

Read the commits together with the full diff to write an accurate description.

## Step 2 — Branch naming

Pattern: `<owner>/<type>/<scope>/<short-description>`.

| Part                | Rule                                                                                                                                            |
|---------------------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| `owner`             | Your handle: `gh api user --jq .login`, or `git config user.name` lowercased and hyphenated. Omit if the repo's convention has no owner prefix. |
| `type`              | `feat`, `fix`, `refactor`, `chore`, `test`, `docs`                                                                                              |
| `scope`             | Component or package changed (`parser`, `auth`)                                                                                                 |
| `short-description` | Lowercase, hyphen-separated                                                                                                                     |

**The repo's own convention wins.** Check recent branches
(`git branch -r --sort=-committerdate | head`) and CONTRIBUTING.md; follow a
mandated scheme such as `<ticket-id>-description` instead.

If the current branch does not match, rename it:
`git branch -m <old-name> <owner>/<type>/<scope>/<short-description>`.

## Step 3 — Quality gates

Detect the repo type and run its checks before committing. Fix failures first.

| Signal                                             | Check                                                      |
|----------------------------------------------------|------------------------------------------------------------|
| A stack check skill is installed (e.g. `go-check`) | Use it; it scopes to affected packages                     |
| `go.mod`                                           | `golangci-lint run` then `go test`, on affected packages   |
| `package.json`                                     | `npm test` or `yarn test`, plus the lint script if present |
| `Cargo.toml`                                       | `cargo clippy` then `cargo test`                           |
| `pyproject.toml` / `setup.py`                      | The project's lint, then `pytest`                          |
| `Makefile` with a test target                      | `make test` (and `make lint` if present)                   |
| None                                               | Skip and tell the user                                     |

Then run a `/simplify`-style review skill if one is available, and fix its
findings before committing.

## Step 4 — Commit (if needed)

Skip when nothing is uncommitted. Otherwise follow `/commit` (stage named files,
never `git add -A`; never amend a commit already on origin), with the message:

- Conventional Commits: `<type>(<scope>): <imperative description>`, first line under 72 characters.
- A body explaining *why*, for non-trivial changes.
- The co-author trailer with the current model name.

```bash
git commit -m "$(cat <<'EOF'
<type>(<scope>): <description>

<optional body>

Co-Authored-By: Claude <model-name> <noreply@anthropic.com>
EOF
)"
```

## Step 5 — Discover repo conventions

Look in parallel for:

- **PR template**: `.github/PULL_REQUEST_TEMPLATE.md`, `.github/pull_request_template.md` or `docs/pull_request_template.md`.
- **Contributing guide**: `CONTRIBUTING.md`.

Use a template as the skeleton, but **omit any section that would be empty**.
Tick a contribution-criteria checkbox that references CONTRIBUTING.md (`- [x]`).

If CONTRIBUTING.md restricts AI in PR text (descriptions, review comments), use
[human-written description](#human-written-description) instead of drafting the body.

Without a template:

```markdown
## Summary

- <bullet points>

## Test plan

- <bullet points>
```

## Step 6 — Push and create the PR

```bash
git push -u origin <branch-name>
```

The title is the conventional-commit headline. **Stacked PR** (branch cut from
another feature branch, not the default): pass `--base <parent-branch>` and add a
"Stacked on" section linking the parent PR.

### Body constraints

Check the draft against every row *before* `gh pr create`; rewrite on any miss.

| Constraint          | Rule                                                                                                                          |
|---------------------|-------------------------------------------------------------------------------------------------------------------------------|
| Bullet              | One rendered line: ≤120 chars, one clause (no "X. Y." or "X — Y because Z").                                                  |
| Summary             | 3–5 bullets, each saying *why* or naming the minimum observable change. No code, file paths or method names.                  |
| Test plan           | 3–4 bullets, each naming a verification *run* (`go test`, `golangci-lint`, "smoke test in staging"). No narrative or caveats. |
| Sub-bullets         | None. Promote the content to a top-level bullet, or cut it.                                                                   |
| Length              | ≤15 rendered lines.                                                                                                           |
| Caveats, follow-ups | Omit ("deferred X to PR Y"); they belong in the commit body or an issue.                                                      |

Same change, BAD then GOOD:

```text
BAD
- Wires up a cache layer so repeated lookups skip the DB round-trip — the
  resolver now checks the LRU before querying, and falls through on a miss,
  which cuts p99 latency on the hot path.
- `resolve()` consults `cache.Get(key)` first and on a miss populates the entry
  via `cache.Set(key, val, ttl)`; the TTL is read from config so ops can tune
  it without a redeploy, defaulting to 5m.
- Verified locally by hammering the endpoint with a loop of 10k requests and
  eyeballing the latency histogram in the debug dashboard, which looked flat.

GOOD
- Adds an LRU cache to the resolver so repeated lookups skip the DB round-trip.
- TTL is config-driven (default 5m) — tunable without a redeploy.
- Verified: `go test ./resolver/...` and a 10k-request local load loop.
```

```bash
gh pr create --title "<type>(<scope>): <description>" --body "$(cat <<'EOF'
<filled-in template, empty sections omitted>
EOF
)"
```

If the created body still breaks a constraint, fix it with `gh pr edit <N> --body …`.

## Step 7 — Report

Return the PR URL.

## Post-push scanner false positives

Scanners such as Codacy and Semgrep run only after push, not in local lint. If
one flags a false positive, suppress it with the scanner's inline directive
naming the exact rule id, not a freeform comment. For Go with Codacy/Semgrep, the
`go-check` skill has the verified form, a rule-id table and a lookup recipe.

## Human-written description

For repos whose CONTRIBUTING.md forbids an LLM-written PR description. The human
writes the prose; the skill only structures and proofreads it. Branch, commit and
title steps are unchanged. Skip the Step 6 body constraints, and never fix the body with `gh pr edit`.

1. Give the user a facts sheet in chat only: what changed, and the test and lint results. Do not draft or pre-fill the Summary or Test plan.
2. Ask what to put in each template section: checkboxes, refs, AI disclosure, Summary, Test plan. Never tick a box (AI disclosure included) on the user's behalf.
3. Build the body from the answers verbatim: template order, empty optional sections removed.
4. Run an English pass on the user's text (grammar, spelling, articles, punctuation, code formatting) without changing wording or content, and show what changed. No need to ask first.
5. Create with `gh pr create --body-file <temp file>`.

Never write PR review comments or run automated reviews that post. Local review stays allowed.
