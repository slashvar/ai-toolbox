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

General-purpose pull request workflow. Adapts to any repo by discovering its PR
template and contributing guidelines at runtime.

---

## Step 1 — Understand the changes

Run these in parallel:

```bash
git status                          # untracked + modified files
git diff HEAD                       # staged + unstaged changes
git log --oneline -10               # recent commit style
git log --oneline main..HEAD        # commits on this branch (if any)
```

Read the full diff carefully — you need it to write an accurate description.

## Step 2 — Branch naming

Use the pattern `<owner>/<type>/<scope>/<short-description>`.

- **owner**: your handle — derive it once from `gh api user --jq .login` (or
  `git config user.name`, lowercased/hyphenated). This namespaces your branches
  on shared remotes. Omit it if the repo's own convention doesn't use an owner
  prefix (see below).
- **type**: `feat`, `fix`, `refactor`, `chore`, `test`, `docs`
- **scope**: the component or package being changed (e.g., `parser`, `auth`)
- **short-description**: lowercase, hyphen-separated

**Defer to the repo's own convention if it has one.** Check recent branch names
(`git branch -r --sort=-committerdate | head`) and CONTRIBUTING.md — if the
project mandates a different scheme (e.g. `<ticket-id>-description`), follow that
instead of this default.

Check the current branch name. If it doesn't match the chosen pattern, rename it:

```bash
git branch -m <old-name> <owner>/<type>/<scope>/<short-description>
```

### Verify the branch forks from the intended base

Before going further, confirm this branch was actually cut from the right base.
With multiple terminals or sessions open, `HEAD` can shift silently, so a branch
created "off main" can inherit another feature branch as its base — the PR then
carries commits that don't belong to it and shows an inflated diff.

```bash
git fetch --quiet origin
git log --oneline origin/<default-branch>..HEAD    # commits this branch adds
```

The listed commits should be *only* the ones belonging to this change. If you
see unrelated commits (another feature's work, someone else's commits), the
branch was cut from the wrong base. Stop and tell the user — do not open a PR
off the wrong base. The usual fix is to re-create the branch from the intended
base and cherry-pick or re-apply this change:

```bash
git checkout <default-branch> && git pull
git checkout -b <owner>/<type>/<scope>/<short-description>
# then re-apply the change (cherry-pick the relevant commits, or rebase --onto)
```

**Exception — intentional stacked PR.** If the branch is *meant* to stack on a
parent feature branch (see Step 6), the parent's commits appearing here is
expected. Confirm the extra commits are exactly the parent's, not something
unrelated.

## Step 3 — Quality gates

Before committing, run the quality pipeline appropriate for the current repo.
Detect the repo type by checking project markers, then apply the matching
strategy. Fix any failures before proceeding.

| Signal | Check action |
|--------|-------------|
| A repo/language-specific check skill is installed (e.g. `go-check`) | Prefer it — scoped to affected packages |
| Has `go.mod` | `golangci-lint run ./...` then `go test ./...` on affected packages |
| Has `package.json` | `npm test` or `yarn test` (and lint script if present) |
| Has `Cargo.toml` | `cargo clippy` then `cargo test` |
| Has `pyproject.toml` / `setup.py` | the project's lint + `pytest` |
| Has `Makefile` with a test target | `make test` (and `make lint` if present) |
| None of the above | Skip checks, note it to the user |

Then, if a `/simplify`-style review skill is available, run it to review changes
for reuse, quality, and efficiency. Fix any findings before committing.

## Step 4 — Commit (if needed)

If there are uncommitted changes, stage and commit them. Follow these rules:

- Stage specific files (`git add <files>`), never `git add -A` or `git add .`
- Use Conventional Commits: `<type>(<scope>): <imperative description>`
- Keep the first line under 72 characters
- Add a body for non-trivial changes explaining *why*
- Always include the co-author trailer with the current model name
- Never amend a commit that already exists on origin — create a new commit
- Use a HEREDOC for the message:

```bash
git commit -m "$(cat <<'EOF'
<type>(<scope>): <description>

<optional body>

Co-Authored-By: Claude <model-name> <noreply@anthropic.com>
EOF
)"
```

If there are already commits on the branch and nothing is uncommitted, skip this step.

## Step 5 — Discover repo conventions

Look for these files in the repo root (check in parallel):

1. **PR template**: `.github/PULL_REQUEST_TEMPLATE.md`, `.github/pull_request_template.md`, or `docs/pull_request_template.md`
2. **Contributing guide**: `CONTRIBUTING.md`

These inform how the PR body is structured. If a PR template exists, use it as
the skeleton — but **omit any section that would be empty** (no blank headings).
If the template has a contribution criteria checkbox that references
CONTRIBUTING.md, check it (`- [x]`).

If no PR template exists, use this default:

```markdown
## Summary

- <bullet points>

## Test plan

- <bullet points>
```

## Step 6 — Push and create PR

```bash
git push -u origin <branch-name>
```

Then create the PR. The title should be the conventional commit headline
(`<type>(<scope>): <description>`).

**Stacked PRs:** If the branch was created on top of another feature branch
(not `main`/`master`), set `--base` to the parent branch. Add a
"Stacked on" section to the body linking the parent PR.

### PR body — hard constraints (enforce before calling `gh pr create`)

These are not "guidelines" — they are constraints. Self-check the drafted body
against this list. If any check fails, rewrite before submitting.

- **Each bullet renders as ONE line.** Source rule: ≤120 chars per bullet,
  no multi-clause bullets (no "X. Y." or "X — Y because Z"). If a bullet wraps
  on a typical PR page (~100 chars wide rendered), split or shorten.
- **Summary: 3–5 bullets max.** Each answers *why* (what problem it solves)
  or names the minimum observable change. No implementation walkthrough —
  no code snippets, no file paths, no method names.
- **Test Plan: 3–4 bullets max.** Each names a verification *run* (e.g.,
  `go test`, `golangci-lint`, "smoke test in staging"). No narrative,
  no caveats, no deferred-work notes.
- **No sub-bullets.** Reviewers skim — a sub-bullet means the parent bullet
  was already too long. Promote sub-bullet content to its own top-level
  bullet, or cut it.
- **Total body: ≤15 lines rendered.** If it's longer, you're over budget.
- **Caveats and follow-ups** (e.g., "deferred X to PR Y"): omit from the
  PR body. They belong in the commit body, an issue, or a tracking note.

Worked example — same change, BAD then GOOD:

BAD (3 multi-clause bullets, implementation detail, internal narrative):
```
- Wires up a cache layer so repeated lookups skip the DB round-trip — the
  resolver now checks the LRU before querying, and falls through on a miss,
  which cuts p99 latency on the hot path.
- `resolve()` consults `cache.Get(key)` first and on a miss populates the entry
  via `cache.Set(key, val, ttl)`; the TTL is read from config so ops can tune
  it without a redeploy, defaulting to 5m.
- Verified locally by hammering the endpoint with a loop of 10k requests and
  eyeballing the latency histogram in the debug dashboard, which looked flat.
```

GOOD (same information, one-line bullets, why-focused):
```
- Adds an LRU cache to the resolver so repeated lookups skip the DB round-trip.
- TTL is config-driven (default 5m) — tunable without a redeploy.
- Verified: `go test ./resolver/...` and a 10k-request local load loop.
```

```bash
gh pr create --title "<type>(<scope>): <description>" --body "$(cat <<'EOF'
<filled-in template with bullet points, empty sections omitted>
EOF
)"
```

After running `gh pr create`, if the rendered body fails any of the
constraints above, immediately update the PR with `gh pr edit <N> --body …`.

## Step 7 — Report

Return the PR URL to the user.

## Post-push CI gotcha — static-analysis false positives

Some checks (Codacy/Semgrep and similar SaaS scanners) run only after push, not
in local lint. If one flags a finding you believe is a false positive, suppress
it with the scanner's own inline directive naming the exact rule id — not a
freeform comment. For Go + Codacy/Semgrep specifically, the `go-check` skill has
the verified suppression form, the rule-id table, and the search recipe for
finding new ids.
