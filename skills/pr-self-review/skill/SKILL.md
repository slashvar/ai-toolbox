---
name: pr-self-review
description: Cold-read self-review of a pull request — spawn a subagent that hasn't seen the design conversation, plan files, or project memory, capture its findings, triage in bulk, apply the accepted fixes as a single commit, then offer a re-review pass. Use whenever you want to review a PR the way an external reviewer would, before requesting human review. Triggers on "self-review", "cold review", "review my PR", "review as external", "/pr-self-review". Do NOT use for reviewing someone else's PR, or for a one-shot pre-commit check of the working tree (use a working-tree review for that).
---

# pr-self-review

Two-phase skill: a cold-read review pass against an opened PR, followed by a
bulk-triage-then-batch-apply fix pass. The reviewer is a subagent with **no
visibility into the project's design conversation, plan files, or
project-scoped memory** — by construction. The goal is to catch the
assumptions and missing context that *you* won't catch precisely because you
designed the change.

The skill is **language- and org-agnostic**. It auto-detects the project's
stack (Step 2) and loads a matching review profile so the reviewer applies the
right idioms and skips whatever your linter already covers. Unknown stack →
generic review, no failure.

This skill is **distinct from**:

- **A heavy multi-agent review** (security, performance, etc.) — if your setup
  has one, use it as the final gate before flipping a PR ready, not as the
  day-to-day self-review loop.
- **A working-tree review** — that reviews uncommitted changes pre-commit.
  `pr-self-review` operates on an opened PR, including its title/body.

---

## Step 1 — Identify the PR and check preconditions

Argument shape: `/pr-self-review` (uses current branch) OR `/pr-self-review <pr-number>`.

```bash
# If a PR number was passed, switch to that branch first.
if [ -n "$PR_NUM" ]; then
  gh pr checkout "$PR_NUM"
else
  PR_NUM=$(gh pr view --json number --jq .number 2>/dev/null || true)
fi

# One metadata fetch covers preconditions, the brief, and the user summary —
# the PR is immutable for the duration of the skill, so don't re-query per field.
META=$(gh pr view "$PR_NUM" --json number,state,title,body,baseRefName,headRefName,isDraft,url)
```

Hard pre-conditions, abort with a clear message if any fail:

1. **PR must exist and be OPEN.** `echo "$META" | jq -r .state` must be
   `OPEN`. Closed/merged → abort.
2. **HEAD must equal `origin/<head-ref>`.** Otherwise the reviewer would
   review stale state. Compare both SHAs in one call:
   `git rev-parse HEAD "origin/$(echo "$META" | jq -r .headRefName)"`. If
   local is ahead, suggest the user push first.
3. **Working tree clean.** If `git status --short` is non-empty,
   `git stash push -u` and remember to pop at the end of the skill. Don't
   stash silently mid-loop.

Surface the resolved PR (number, title, head branch, base branch, draft
status) to the user before doing anything mutating — all of these are already
in `$META`. Then continue.

## Step 2 — Detect the stack and assemble the review profile

Sniff the repo root for the first matching marker, then read the matching
section of `references/review-profiles.md`:

| Marker file | Profile |
|---|---|
| `go.mod` | Go |
| `Cargo.toml` | Rust |
| `package.json` | JavaScript / TypeScript |
| `pyproject.toml`, `setup.py`, `requirements.txt` | Python |
| `pom.xml`, `build.gradle` | Java / Kotlin |
| `Gemfile` | Ruby |
| *(none of the above)* | Generic only |

```bash
# Cheap detection — first match wins; fall through to generic.
for m in go.mod Cargo.toml package.json pyproject.toml setup.py pom.xml build.gradle Gemfile; do
  [ -f "$m" ] && { echo "marker: $m"; break; }
done
```

The profile contributes a **focus addendum** (idioms to check) and a **skip
addendum** (what the project's linter/formatter already enforces, so the
reviewer doesn't waste findings on it). These layer on top of the generic
core in Step 3. If no marker matches, use the generic core alone and say so to
the user.

Detection sniffs the repo root only. In a monorepo or polyglot repo the root
marker may not match the language the diff actually touches — if the changed
files are clearly in a different stack than the detected profile, prefer the
profile matching the diff (or fall back to generic) and tell the user.

## Step 3 — Run the cold-read review

Capture the inputs in a self-contained brief and spawn one general-purpose
Agent.

**Context-suppression rule (load-bearing).** The reviewer must NOT see:
- the design conversation that produced this PR,
- plan / design files for this change,
- any project-scoped memory or notes specific to this codebase.

The reviewer MAY use generic, cross-project coding conventions (style guides,
language idioms). The point is a genuinely cold read: the reviewer should
react to the diff the way someone encountering it for the first time would. If
you run a memory system, exclude project-scoped entries from the subagent and
allow only generic-convention ones.

```bash
# Reuse the $META blob from Step 1; only the diff needs a fresh call.
PR_TITLE=$(echo "$META" | jq -r .title)
PR_BODY=$(echo "$META" | jq -r .body)
PR_BASE=$(echo "$META" | jq -r .baseRefName)
DIFF=$(gh pr diff "$PR_NUM" --patch)
```

Stage the brief at a temp path (e.g. `${TMPDIR:-/tmp}/pr-self-review-input-<pr>.md`)
so the Agent has a single artifact to read — this avoids token-budget blowups
from passing huge diffs inline. The brief includes:

```markdown
# Inputs

- PR: #<pr> — "<title>"
- Base: <base-ref>
- Diff (unified, as `gh pr diff` produced it)
- PR body (full markdown)

# Reviewer instructions

You are a senior engineer reading this PR cold. You have NOT seen the design
conversation, the plan/design files, or any project-specific context. Generic,
cross-project coding conventions are fair game.

Focus on (generic core):
- Hidden assumptions and undocumented invariants
- Test coverage gaps (especially error paths, edge cases the diff hints at)
- Naming clarity, especially for public / exported APIs
- PR body: does it explain WHAT changed and WHY, enough to review the diff?
- Anything that would confuse a future maintainer reading this six months from now

Focus on (language profile addendum — from references/review-profiles.md):
<paste the FOCUS addendum for the detected stack, or omit if generic>

Skip:
- Subjective style preferences
- Speculative refactors not justified by current code
- Premature abstraction suggestions
- Commit-message critique
- Anything the project's linter/formatter would surface (those run separately):
<paste the SKIP addendum for the detected stack, or omit if generic>

Output: write `${TMPDIR:-/tmp}/pr-review-<pr>.md` with the structured findings
below. Render the same content as your tool reply so it shows in the conversation.

# Output schema

If no findings: write a one-line file containing `_No findings._` and reply
with the same.

Otherwise, one block per finding, numbered from 1:

## Finding N: <one-line title>

**Severity:** blocker | suggestion | nit
**File:** <path>:<line>  (or "PR body" / "PR title")
**Issue:** 2–3 sentences explaining the concern.
**Suggested fix:** concrete change. Quote the new lines / new wording.
```

Agent invocation:

```text
Agent({
  subagent_type: "general-purpose",
  description: "Cold-read PR self-review",
  prompt: <the brief above, fully self-contained>
})
```

The brief tells the agent to write `pr-review-<pr>.md`. Verify the file exists
after the Agent returns. If empty / missing, treat as an Agent failure and
surface to the user.

## Step 4 — Triage in bulk

Read `pr-review-<pr>.md` and render it in the conversation (it's already markdown).

If the file says `_No findings._`, skip to Step 7 with the message
"✓ no findings, PR looks clean."

Otherwise, ask the user **once**, with the full list visible, to triage:

```
Triage example:
  - fix 1,3,5 — apply the suggested fix as-is
  - skip 2 (don't agree with the reviewer)
  - discuss 4 (let's talk before deciding)
  - edit 6 — accept but I'll dictate a different fix
```

Capture the user's reply. The triage is a single round-trip; don't drag
per-finding.

In auto-mode, still pause for this triage — the human judgment is the
load-bearing decision this skill is designed around. Bypassing it makes the
skill equivalent to auto-apply, which we explicitly chose against.

## Step 5 — Discuss flagged findings, finalize the apply list

For each finding marked `discuss` or `edit`, have a short back-and-forth with
the user, then collapse it back into the apply set (`fix`) or the skip set
(`skip`).

At this step every finding is either `fix` or `skip`. Update
`pr-review-<pr>.md` in place:

- For `fix` findings: leave as-is.
- For `skip` findings: append `**Resolution:** skipped — <one-line reason from
  the user, if given>` under the finding.

This keeps the review file as a durable record of what the self-review caught
and why each finding was or wasn't addressed.

## Step 6 — Apply fixes in a batch

For each `fix` finding, make the suggested change using the appropriate tool
(Edit for code edits, `gh pr edit` for PR body / title changes). Apply all of
them before showing the user the diff. Do not commit yet.

After the apply pass:

1. Show `git status --short` (any unstaged changes the apply produced).
2. Show `git diff` (the actual changes).
3. Ask the user to confirm the diff looks right before committing.

If any apply step fails (Edit can't find the old_string because the suggested
fix was inaccurate, etc.), surface the failure to the user finding-by-finding —
don't silently drop fixes.

## Step 7 — Commit and push

If at least one finding was applied:

1. Propose a commit message: `chore(<scope>): address self-review findings`
   where `<scope>` is inferred from the most-touched directory. Show it to the
   user; they may accept, edit, or replace. (If the repo doesn't use
   conventional commits, fall back to its observed style.)
2. Commit using a HEREDOC for message safety.
3. `git push` (no force — `pr-self-review` only operates on the existing HEAD
   line of history).

If zero findings were applied (all skipped, or no findings to begin with):
skip the commit. Show a one-line summary of what was triaged and exit.

## Step 8 — Offer re-review (don't force)

If at least one fix was committed:

```
✓ committed <commit-sha>. Run another review pass against the new HEAD? (y/N)
```

Default is N. If the user accepts:

1. Archive the current `pr-review-<pr>.md` to
   `pr-review-<pr>.<timestamp>.md` so history isn't lost.
2. Loop back to Step 2 (re-detect + re-run against the fresh HEAD).

If no re-review: exit cleanly. If the user auto-stashed at Step 1, `git stash
pop` now and tell them.

---

## Decisions locked in this skill (do not relitigate)

| Decision | Choice |
|---|---|
| Trigger | Standalone invocation, not auto-chained from a PR-creation skill |
| Context suppression | Principle-based: design conversation, plan files, and project-scoped memory excluded; generic-convention memory allowed |
| Stack handling | Auto-detect by marker file → load profile from `references/review-profiles.md`; generic fallback |
| Review depth | Light single-Agent cold-read pass, not a heavy multi-agent flow |
| Output location | `${TMPDIR:-/tmp}/pr-review-<pr>.md` (saved file + rendered to terminal) |
| Fix interaction | Bulk triage in one round-trip, then batch apply |
| Re-review | Offered after commit, defaults to no |
| PR identification | Current branch by default; optional `<pr-number>` arg |
| Review scope | Diff + PR body (no commit messages, no per-pkg meta) |
| Reviewer persona | "Senior engineer reading cold" + detected-stack idioms + explicit skip list |

## Anti-patterns this skill exists to prevent

1. **Reviewing a PR using the same context that wrote it.** That misses the
   exact class of issue an external reviewer catches — the ones that look
   obvious *to you* and confusing to everyone else.
2. **Auto-applying reviewer findings.** The reviewer is a fresh agent without
   your project context, by design. Some findings will misread intent. Triage
   is load-bearing.
3. **Per-finding triage drag.** Showing each finding individually and asking
   "fix or skip?" turns a 10-finding review into 10 round-trips. Bulk triage
   stays interactive without being chatty.
4. **Forgetting the skipped findings.** Skipped findings annotated with their
   reason in the review file give future-you (or a real reviewer) evidence that
   the issue was considered.
5. **Re-running the review automatically after a fix.** The new run sees
   different code and produces different findings; auto-loops can flip-flop or
   burn agent calls. Offer, don't force.
6. **Posting findings to GitHub by default.** Self-review noise on a PR you may
   still iterate on is pollution. Local-only by default; add a `--post` flag
   later if there's real value.
7. **Hardcoding one language's idioms.** A Go-only skip list flags as findings
   things that are normal in Python (and vice-versa). The profile layer keeps
   the reviewer accurate per stack.
