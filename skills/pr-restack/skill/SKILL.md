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

Chain-rebase a stack of DRAFT PRs: the bottom branch onto the default branch,
each other branch onto its parent's tip, force-pushing (with lease) as you go.
Child PRs then show only their own commits.

Rebase + force-push is the counterpart to `/pr-sync` (merge, no history
rewrite, safe for ready PRs). Do not mix them: merging the default branch into
each draft inflates every child's diff and duplicates history. If the stack mixes
drafts and ready PRs, stop and reconcile; never switch strategy mid-stack.

## Step 0 — Pre-conditions

Abort and surface the reason if any fails, before rebasing anything.

1. **Every PR is DRAFT.** Force-pushing a non-draft overwrites reviewer state and re-triggers approvals. If a PR was marked ready mid-stack, use `/pr-sync` for it and ask before continuing.
2. **Clean working tree.** If `git status --short` prints anything, `git stash push -u` and pop on every exit path. A checkout over uncommitted changes can abort silently, leaving the loop on the wrong branch.
3. **Default branch known:** `gh repo view --json defaultBranchRef --jq .defaultBranchRef.name`.

## Step 1 — Discover the stack

Start from the **top**: the branch the user named, else `git branch --show-current`.
Follow `baseRefName` down until it equals the default branch:

```bash
gh pr view <branch-or-number> --json number,headRefName,baseRefName,isDraft,mergeable,state
```

Order the chain bottom → top (`main ← PR_1 ← PR_2 ← … ← PR_N`) and show it as a
numbered list **before** any mutation; the user may have meant another top.

| Condition                       | Action                                            |
|---------------------------------|---------------------------------------------------|
| A PR has `state != OPEN`        | Abort; name the closed/merged PR.                 |
| A PR has `isDraft == false`     | Abort (Step 0).                                   |
| A branch is not fetched locally | `git fetch origin <branch>:<branch>`              |
| A `headRefName` has no PR       | Abort; ask whether the stack is what they expect. |

## Step 2 — Pre-flight CI status

As in `/pr-sync`, surface failing checks before rewriting history, so a
pre-existing failure is not buried:

```bash
for pr in <numbers...>; do
  gh pr view "$pr" --json number,statusCheckRollup --jq '{n: .number, fails: [.statusCheckRollup[]? | select(.conclusion | IN("FAILURE","CANCELLED","TIMED_OUT","STARTUP_FAILURE")) | .name]}'
done
```

If any branch is red, list the failures and ask whether to proceed. A draft deep
in the stack may be legitimately red because it needs an unmerged sibling; the
user knows which failures are expected.

## Step 3 — Rebase chain, bottom-up

```bash
git fetch origin
```

For each PR, bottom first:

**3a. Switch.** `git checkout <head-ref>`. If it fails (dirty tree, divergent local copy), stop and surface the error. Do not `git stash` mid-loop without asking.

**3b. Target.** The bottom branch rebases onto the default branch; every other branch onto the previous branch in the stack. Always use the `origin/<target>` ref, so the parent pushed in 3e is the source of truth.

**3c. Rebase.**

```bash
git rebase origin/<target>
```

Do not pipe it; the pipe hides the exit code. On conflicts, **stop the whole loop** and show:

1. Conflicted files: `git diff --name-only --diff-filter=U`
2. Auto-merged but staged files: `git status --short | awk '/^M /'`
3. About 20 lines of `git diff` around each conflict marker.

If the conflicts are duplicates of commits the new base already contains (many
near-identical files, mostly staged `M ` rather than `UU`), take the base's
version for **every** listed file, staged-`M` ones included:

```bash
git status --short | awk '/^[MADRUC][MADRUC ]/ {print $2}' | xargs git checkout HEAD --
```

This discards the branch's version, so use it only when every file is a duplicate.
Never auto-resolve otherwise. Do not `git rebase --abort` unless asked. The user
resolves, then re-invokes `/pr-restack`; Step 1 finds the resume point.

**3d. Build before pushing.** A clean rebase does not mean the code compiles:
rebasing past an API change breaks the build with no conflict signal. Build what
the branch touches relative to `origin/<target>`:

- If a stack check skill is installed (e.g. `go-check`), use it on the touched packages.
- Go without one: run the changed-package build from `/pr-sync` Step 5 with `origin/<target>` as the base. In short, `go build` each changed dir that has no `//go:build` file, and vet tagged dirs with `go vet -tags <tag>`.
- Other stacks: the repo's equivalent compile check. If none is fast, note that no local build ran for this branch.

If the build fails, stop the loop, show the error and ask the user to fix on this
branch and re-invoke. Never push a branch that compiled before the rebase but not after.

**3e. Push.**

```bash
git push --force-with-lease
```

Never bare `--force`: the lease refuses to overwrite commits that landed on
origin since your last fetch (CI bots, reviewers, another machine). If the push
is rejected, stop and tell the user origin moved; never retry with `--force`.
On success, the next branch rebases onto this one's fresh `origin/` ref.

## Step 4 — Final status report

```bash
for pr in <numbers...>; do
  gh pr view "$pr" --json number,title,headRefName,baseRefName,isDraft,mergeable,statusCheckRollup,url
done
```

Print one summary:

```text
Stack restacked successfully:

  #N1  <owner>/feat/.../bottom        base: main                       MERGEABLE  draft
  #N2  <owner>/feat/.../middle        base: <owner>/feat/.../bottom    MERGEABLE  draft
  #N3  <owner>/feat/.../top           base: <owner>/feat/.../middle    MERGEABLE  draft

CI runs are queued on origin — re-check in a few minutes.
```

Pop the Step 0 stash and tell the user. Flag any PR that is `CONFLICTING`: a
child usually resolved a conflict against an old parent version the rebase did
not preserve.
