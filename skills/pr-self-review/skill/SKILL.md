---
name: pr-self-review
description: Cold-read self-review of a pull request — spawn a subagent that hasn't seen the design conversation, plan files, or project memory, capture its findings, triage in bulk, apply the accepted fixes as a single commit, then offer a re-review pass. Use whenever you want to review a PR the way an external reviewer would, before requesting human review. Triggers on "self-review", "cold review", "review my PR", "review as external", "/pr-self-review". Do NOT use for reviewing someone else's PR, or for a one-shot pre-commit check of the working tree (use a working-tree review for that).
---

# pr-self-review

A cold-read review of an opened PR, then a bulk-triage-then-batch-apply fix
pass. The reviewer is a subagent with **no visibility into the design
conversation, plan files, or project-scoped memory**. It catches the
assumptions and missing context that you miss because you designed the change.

The skill is language- and org-agnostic. It detects the stack (Step 2) and
loads a matching review profile; an unknown stack gets the generic review.

It is distinct from:

- **A heavy multi-agent review** (security, performance, …): use it as the
  final gate before marking a PR ready, not as the day-to-day loop.
- **A working-tree review**: that covers uncommitted changes. This skill covers
  an opened PR, including its title and body.

## Conventions

**Scratch root.** `SCRATCH="${TMPDIR:-/tmp}"`. If the user's configuration
names another scratch directory, use that instead. Every temp path below hangs
off `$SCRATCH`.

**Rules for reviewer findings.** These apply at every step below.

| Rule                 | Detail                                                                                                                                                                                                                                                 |
|----------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Settled decisions    | List them in the brief as "do NOT re-propose": choices the user already made, explicit human-reviewer requests, and findings skipped in an earlier pass. State outcomes only, not the design conversation.                                             |
| Verify before triage | Check each finding against the code and tests before the user sees it. Agents misread: a "needless" helper that is used, "duplicate" test cases that differ, a logical reduction that is invalid.                                                      |
| Premise check        | Before recommending a fix, check its premise: does the suggested helper exist and fit, and will it survive (is the PR or the codebase removing it)?                                                                                                    |
| Human wins           | An explicit request from a human reviewer beats an agent's simplification.                                                                                                                                                                             |
| Account for skips    | Every skipped finding, including those dropped by verification, gets a one-line reason.                                                                                                                                                                |
| Repo policy          | If the repo forbids AI-written PR descriptions or comments (check `CONTRIBUTING`, the PR template, agent-instruction files), never post a comment or run `gh pr edit`. Propose the text and let the user apply it. Findings are local-only by default. |

## Step 1 — Identify the PR

Argument: none (current branch) or `<pr-number>`.

1. **Working tree clean.** If `git status --short` is non-empty, tell the user,
   then `git stash push -u` and pop at the end (Step 8). Never stash silently
   mid-loop.
2. Resolve the PR. One metadata fetch serves the checks, the brief, and the
   summary; the PR does not change during the run.

```bash
[ -n "$PR_NUM" ] && gh pr checkout "$PR_NUM"
META=$(gh pr view ${PR_NUM:+"$PR_NUM"} --json number,state,title,body,baseRefName,headRefName,isDraft,url) || exit 1
PR_NUM=$(jq -r .number <<<"$META")
HEAD_REF=$(jq -r .headRefName <<<"$META")
git fetch -q origin "$HEAD_REF" && git rev-parse HEAD "origin/$HEAD_REF"
```

Abort with a clear message if:

- **No PR, or state is not `OPEN`.**
- **The two SHAs differ.** The reviewer would see stale state. If local is
  ahead, ask the user to push first.

Show the user the PR number, title, head and base branches, and draft status
before anything mutating.

## Step 2 — Detect the stack

First marker found at the repo root wins. Read the matching section of
`references/review-profiles.md`.

| Marker file                                      | Profile                     |
|--------------------------------------------------|-----------------------------|
| `go.mod`                                         | Go                          |
| `Cargo.toml`                                     | Rust                        |
| `package.json`                                   | JavaScript / TypeScript     |
| `pyproject.toml`, `setup.py`, `requirements.txt` | Python                      |
| `pom.xml`, `build.gradle`                        | Java / Kotlin               |
| `Gemfile`                                        | Ruby                        |
| *(none)*                                         | Generic only; tell the user |

A profile adds a **focus addendum** (idioms to check) and a **skip addendum**
(what the project's linter already enforces). Both layer on the generic core in
Step 3.

Detection sniffs the root only. If the changed files are clearly in another
stack (monorepo, polyglot repo), use the profile matching the diff, or generic,
and tell the user.

## Step 3 — Run the cold-read review

**Context suppression (load-bearing).** The reviewer must not see the design
conversation, plan or design files, or project-scoped memory or notes. It may
use generic, cross-project conventions (style guides, language idioms). If you
run a memory system, exclude project-scoped entries from the subagent.

Stage the PR title, body, and diff in one file, so the agent reads a single
artifact and no huge diff is passed inline:

```bash
BRIEF="$SCRATCH/pr-self-review-input-$PR_NUM.md"
{
  printf '# PR #%s: %s\n\nBase: %s\n\n## Body\n\n%s\n\n## Diff\n\n' \
    "$PR_NUM" "$(jq -r .title <<<"$META")" "$(jq -r .baseRefName <<<"$META")" "$(jq -r .body <<<"$META")"
  gh pr diff "$PR_NUM" --patch
} >| "$BRIEF"
```

Spawn one `general-purpose` Agent. Its prompt is self-contained:

```markdown
You are a senior engineer reading PR #<pr> cold. You have NOT seen the design
conversation, plan files, or project context. Generic, cross-project coding
conventions are fair game. Read the title, body, and diff in <BRIEF path>.

Focus on:
- Hidden assumptions and undocumented invariants
- Test coverage gaps (error paths, edge cases the diff hints at)
- Naming clarity, especially for public / exported APIs
- PR body: does it explain WHAT changed and WHY, enough to review the diff?
- Anything that would confuse a maintainer six months from now
<FOCUS addendum for the detected stack, if any>

Skip:
- Subjective style preferences
- Speculative refactors and premature abstraction
- Commit-message critique
- Anything the project's linter or formatter surfaces:
  <SKIP addendum for the detected stack, if any>

Settled decisions. Do NOT re-propose:
<settled decisions, or "none">

Output: write <$SCRATCH/pr-review-<pr>.md> and render the same content as your
reply. If there are no findings, write the single line `_No findings._`.
Otherwise one block per finding, numbered from 1:

## Finding N: <one-line title>

**Severity:** blocker | suggestion | nit
**File:** <path>:<line>  (or "PR body" / "PR title")
**Issue:** 2–3 sentences.
**Suggested fix:** concrete change; quote the new lines or wording.
```

Verify the file exists and is non-empty after the Agent returns. If not, treat
it as an Agent failure and tell the user.

## Step 4 — Verify and triage in bulk

Render `pr-review-<pr>.md` in the conversation. If it says `_No findings._`,
skip to Step 7 with "✓ no findings, PR looks clean."

Otherwise apply the **verify** and **premise check** rules to every finding.
Annotate each in the file with `**Check:** confirmed | wrong — <reason>`. A
`wrong` finding is shown to the user as pre-marked `skip`.

Then ask the user **once**, with the full annotated list visible:

```text
fix 1,3,5      apply the suggested fix as-is
skip 2         disagree with the reviewer
discuss 4      talk before deciding
edit 6         accept, but I'll dictate a different fix
```

One round-trip; no per-finding drag. Pause for the triage even in auto mode:
human judgment on findings from a context-free agent is the point of the skill.

## Step 5 — Finalize the apply list

Resolve each `discuss` or `edit` finding in a short exchange, then reduce every
finding to `fix` or `skip`. Update `pr-review-<pr>.md` in place: under each
`skip`, append `**Resolution:** skipped — <one-line reason>`. The file is the
durable record of what the review caught and why each finding was or was not
addressed.

## Step 6 — Apply in a batch

For each `fix`, make the change (Edit for code). PR title and body changes use
`gh pr edit`, unless the **repo policy** rule applies; then show the proposed
text instead. Apply everything before showing the diff, and do not commit yet.

Then show `git status --short` and `git diff`, and ask the user to confirm.
If an apply step fails (the suggested old text is not found, say), report it
per finding; never drop a fix silently.

## Step 7 — Commit and push

If at least one finding was applied:

1. Propose `chore(<scope>): address self-review findings`, with `<scope>` the
   most-touched directory. Follow the repo's commit style if it differs. The
   user may edit it.
2. Commit with a HEREDOC message.
3. `git push`, never forced: the skill only extends the existing history.

Otherwise skip the commit and print a one-line triage summary that lists every
skipped finding with its reason.

## Step 8 — Offer re-review

After a commit, ask: `✓ committed <sha>. Run another review pass? (y/N)`.
Default is N. On yes:

1. Archive the review:
   `mv "$SCRATCH/pr-review-$PR_NUM.md" "$SCRATCH/pr-review-$PR_NUM.$(date +%Y%m%dT%H%M%S).md"`.
2. Re-run from Step 2 with a fresh `$META` and brief. Add this pass's skipped
   findings to the settled decisions.

On no, exit. If Step 1 stashed, `git stash pop` now and say so.

## Design choices

| Choice                                        | Reason                                                                                          |
|-----------------------------------------------|-------------------------------------------------------------------------------------------------|
| Standalone, not auto-chained from PR creation | The user decides when the PR is ready for a cold read                                           |
| One light agent, not a multi-agent flow       | Day-to-day loop; the heavy review is the final gate                                             |
| Bulk triage, then batch apply                 | Ten findings cost one round-trip, not ten                                                       |
| Triage never skipped                          | The reviewer lacks project context, so some findings misread intent; auto-apply would land them |
| Re-review offered, never automatic            | A new run sees new code and finds different things; loops flip-flop and burn calls              |
| Findings stay local                           | Self-review noise posted on a PR still in flux is pollution                                     |
| Skips annotated with reasons                  | Future-you or a human reviewer can see the issue was considered                                 |
| Profile layer per stack                       | One language's skip list flags normal idioms in another                                         |
