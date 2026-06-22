---
name: pr-restack
description: >
  Re-align a chain of stacked DRAFT pull requests by rebasing each branch
  onto its parent branch (not onto main), then force-push-with-lease. Use
  this skill whenever the user has a stack of draft PRs where the children
  have diverged from their parents — typically visible on GitHub as a
  child PR showing commits from an outdated version of its base, an inflated
  diff that includes the parent's content, or "this branch is N commits
  behind <parent-branch>". Triggers on "restack", "rebase the stack",
  "rebase stacked PRs", "/pr-restack", "my stacked PRs are diverging",
  "fix stacked PR history", "child PR shows wrong commits", "realign the
  stack". Force-push is allowed because the PRs are DRAFT.
  Do NOT use when ANY PR in the chain is not a draft (use /pr-sync per PR
  instead — merge-based, no force-push). Do NOT use for a single PR
  against main (use /pr-sync). Do NOT use to create PRs (/pr) or commit
  (/commit).
---

# pr-restack

Re-align a stack of DRAFT pull requests by chain-rebasing each branch onto its
parent. Each PR_n is rebased onto PR_(n−1)'s tip; the bottom of the stack is
rebased onto `main` (or the repo default branch). Then each branch is
force-push-with-lease'd to origin.

This is the rebase counterpart to `/pr-sync`. The two are mutually exclusive:

- **`/pr-sync`** uses **merge**, never rewrites history, safe for ready PRs.
- **`/pr-restack`** uses **rebase + force-push-with-lease**, requires every
  PR in the chain to be **DRAFT**, and keeps the GitHub-rendered history of
  child PRs in sync with their parent.

If you ever need a hybrid (some drafts, some ready), stop and reconcile —
do not silently switch strategies mid-stack.

---

## Step 0 — Hard pre-conditions

Before touching anything:

1. **All PRs in the stack must be DRAFT.** A force-push on a non-draft PR
   overwrites reviewer state and confuses CI. If even one is `isDraft: false`,
   abort and tell the user.
2. **Working tree must be clean.** If `git status --short` is non-empty,
   `git stash push -u` (auto-stash) and remember to pop at the end. This matters
   because `git checkout` over a chain of branches with uncommitted changes can
   silently abort, leaving the loop running on the wrong branch.
3. **Default branch known.** Read it once: `git remote show origin | grep 'HEAD branch'`.

If any of these fails, stop and surface the reason; do not begin rebasing.

## Step 1 — Discover the stack

Starting point is the **top** of the stack: either an explicit branch passed
to the skill, or `git branch --show-current` if none.

Walk down the chain by following each PR's `baseRefName` until the base equals
the default branch:

```bash
gh pr view <branch-or-number> --json number,headRefName,baseRefName,isDraft,mergeable,state
```

Repeat with `baseRefName` as the next argument until `baseRefName` is the
default branch. Build an ordered list, **bottom → top**:

```
[bottom] main ← PR_1 (base) ← PR_2 ← ... ← PR_N [top]
```

For each PR captured, record: number, head ref, base ref, draft flag,
mergeable status, state.

Stop conditions and what to do:

- Any PR has `state != OPEN` → abort, tell the user which PR is closed/merged.
- Any PR has `isDraft == false` → abort with the message from Step 0.
- Any branch in the chain isn't fetched locally → `git fetch origin <branch>:<branch>` (refs only).
- A branch in `headRefName` isn't reachable as a PR (gh returns no PR) → abort,
  ask the user whether the stack is configured the way they expect.

Show the user the discovered stack as a numbered list before doing anything
mutating. They may have meant a different top-of-stack.

## Step 2 — Pre-flight CI status

For the same reason `/pr-sync` surfaces failing checks before merging — to
avoid burying a pre-existing failure under a history rewrite — list any
failing checks across the whole stack:

```bash
for pr in <numbers...>; do
  gh pr view "$pr" --json number,statusCheckRollup \
    --jq '{n: .number, fails: [.statusCheckRollup[]? | select(.conclusion == "FAILURE" or .conclusion == "CANCELLED" or .conclusion == "TIMED_OUT")| .name]}'
done
```

If any branch has pre-existing red CI, show the list and ask whether to
proceed. Do not proceed automatically.

A draft deep in a stack may be legitimately red because it depends on a sibling
branch that isn't merged yet — the user knows which failures are expected. Defer
to them rather than blocking.

## Step 3 — Rebase chain, bottom-up

Fetch once:

```bash
git fetch origin
```

For each PR in the discovered order (bottom-most first):

### 3a. Switch to the branch

```bash
git checkout <head-ref>
```

If checkout fails (dirty tree slipped past Step 0, divergent local copy),
stop and surface the error — do **not** `git stash` mid-loop without
re-confirming with the user.

### 3b. Determine the rebase target

- Bottom branch (base = default branch): `<target>` is the default branch.
- Other branches: `<target>` is the previous branch in the stack.

Always rebase onto the **`origin/<target>`** ref (as written in 3c/3d), not the
local copy, so the just-pushed parent in step 3e is the source of truth.

### 3c. Run the rebase

```bash
git rebase origin/<target>
```

If the rebase succeeds: continue to 3d.

If conflicts: **stop the whole loop.** Show:
1. Files in conflict: `git status --short | awk '/^(UU|AA|UA|AU|DU|UD|DD)/ {print $2}'`
2. Any auto-merged-but-staged files: `git status --short | awk '/^M /'`
3. For each, ~20 lines of `git diff` around the conflict markers.

When the conflicts look like duplicate commits already present on the new base
(high file count, all near-identical, mostly `M ` rather than `UU` — i.e. the
parent already landed these changes), the fix is usually to take HEAD for the
whole set, **including the staged-`M` files** (not just the `UU` conflicts):

```bash
git status --short | awk '/^[MADRUC][MADRUC ]/ {print $2}' | xargs git checkout HEAD --
```

Do not auto-resolve. Do not `git rebase --abort` unless the user asks. The
user resolves, then re-invokes `/pr-restack` — discovery in Step 1 will pick
the right resume point.

### 3d. Build before pushing

A clean rebase does NOT mean the code compiles — rebasing past an API change on
the new base produces broken code with no conflict signal. Run a quick build of
what this branch touches before pushing.

If a repo/language-specific check skill is installed (e.g. `go-check`), prefer
it, scoped to the touched packages. Otherwise build the changed packages
directly. For a Go repo:

```bash
# Build the packages this branch touches relative to its parent.
# dirname maps each changed file to its package dir (a repo-root file -> "."),
# and the sed prefixes "./" so go build treats them as package paths.
REBASE_TARGET=origin/<target>
git diff --name-only "$REBASE_TARGET"...HEAD -- '*.go' \
  | xargs -r -n1 dirname | sort -u | sed 's|^|./|' \
  | xargs -r go build
```

For other languages, substitute the repo's equivalent build/compile check. If
the repo has no fast build check, note in the summary that no local build
verification ran for that branch.

If the build fails: stop the loop, show the error, instruct the user to fix
on this branch and re-invoke. Do **not** push a broken branch — pushing a
branch that compiled before the rebase but not after is the exact failure mode
this step guards against.

### 3e. Push with `--force-with-lease`

```bash
git push --force-with-lease
```

Never bare `--force`. `--force-with-lease` refuses to overwrite if origin
moved since the last `git fetch` — the only safe form of force-push on a
branch that may have CI commits, reviewer pushes, or another machine pushing.

If the push is rejected: stop and tell the user origin has moved. They need
to inspect and decide; do **not** retry with bare `--force`.

If the push succeeds: proceed to the next branch in the loop. The next
branch's rebase target (`origin/<this-branch>`) is now up to date.

## Step 4 — Final status report

After all branches are pushed, print one summary table:

```bash
for pr in <numbers...>; do
  gh pr view "$pr" --json number,title,headRefName,baseRefName,isDraft,mergeable,statusCheckRollup,url
done
```

Format as:

```
Stack restacked successfully:

  #N1  <owner>/feat/.../bottom        base: main                       MERGEABLE  draft
  #N2  <owner>/feat/.../middle        base: <owner>/feat/.../bottom    MERGEABLE  draft
  #N3  <owner>/feat/.../top           base: <owner>/feat/.../middle    MERGEABLE  draft

CI runs are queued on origin — re-check in a few minutes.
```

If the user auto-stashed in Step 0, `git stash pop` now and tell them.

If anything is `CONFLICTING` after the chain, surface it explicitly — usually
means a child branch resolved a conflict against an old parent version that
the rebase didn't preserve.

## Anti-patterns this skill exists to prevent

1. **Merging main into each branch in a stack of drafts** — works, but each
   child's review diff balloons with the merge commit, and GitHub's
   "commits" tab shows duplicated history. Drafts should rebase.
2. **Bare `git push --force`** — overwrites concurrent pushes (CI bots,
   another laptop, the user's prior session).
3. **Pushing without a build check** — a clean rebase past an API change on the
   new base produces broken code with no conflict signal.
4. **Force-pushing a non-draft PR** — clobbers review state and re-triggers
   approval requirements. If the user marked a PR ready mid-stack, fall back
   to `/pr-sync` for that one and ask before continuing.
5. **Resolving conflicts while ignoring staged-`M` files** — when taking HEAD to
   drop duplicate commits, include the staged-`M` files, not just the `UU` ones.
