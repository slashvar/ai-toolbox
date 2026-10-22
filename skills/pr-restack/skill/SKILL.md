---
name: pr-restack
description: >
  Re-align a chain of stacked pull requests onto their parents (not onto
  main): drafts by rebase + force-push-with-lease, ready PRs by merge. Use
  this skill whenever the user has a stack of PRs where the children
  have diverged from their parents — typically visible on GitHub as a
  child PR showing commits from an outdated version of its base, an inflated
  diff that includes the parent's content, or "this branch is N commits
  behind <parent-branch>". Triggers on "restack", "rebase the stack",
  "rebase stacked PRs", "/pr-restack", "my stacked PRs are diverging",
  "fix stacked PR history", "child PR shows wrong commits", "realign the
  stack", "the stack is out of date". Draft branches are rebased and
  force-pushed (with lease); ready-for-review branches in the same stack are
  merged instead, never force-pushed. In a repo that requires GitHub Stacked
  PRs, drives the stack with `gh stack`. Do NOT use for a single PR against
  main (use /pr-sync). Do NOT use to create PRs (/pr) or commit (/commit).
---

# pr-restack

Re-align a stack bottom-up so each child PR shows only its own commits. The
strategy is chosen **per branch**, from that branch's own PR state:

| Branch's PR       | Update onto its target                     | Push                          |
|-------------------|--------------------------------------------|-------------------------------|
| Draft             | rebase                                     | `--force-with-lease`          |
| Ready for review  | merge (as `/pr-sync`), never rebase        | plain push, never forced      |

The target is the default branch for the bottom and the parent's tip for every
other branch. A draft above a ready parent is still rebased, onto the parent's
new tip after the merge. Merging into a draft instead would inflate every child's
diff and duplicate history.

## Step 0 — Pre-conditions

Abort and surface the reason if any fails, before rebasing anything.

1. **Ready PRs sit below drafts.** Rebasing a draft rewrites its commits, so a ready PR above it would need a force-push too. Stop and ask.
2. **Clean working tree.** Tracked changes: `git stash push -u` and pop on every exit path; a checkout over uncommitted changes can abort silently, leaving the loop on the wrong branch. Untracked files that the user keeps on purpose do not block `git`; leave them (see the `gh stack` note in Step 3 for the one tool they block).
3. **Default branch known:** `gh repo view --json defaultBranchRef --jq .defaultBranchRef.name`.
4. **Stack tool.** If the repo's `CONTRIBUTING.md` requires GitHub Stacked PRs (`gh stack`), load the `gh-stack` skill and use the `gh stack` notes below. Otherwise use plain `git` and `gh pr`.

## Step 1 — Discover the stack

Start from the **top**: the branch the user named, else `git branch --show-current`.
Follow `baseRefName` down until it equals the default branch:

```bash
gh pr view <branch-or-number> --json number,headRefName,baseRefName,isDraft,mergeable,state
```

With `gh stack`, read the chain instead with `gh stack view --json` (bottom
first; `needsRebase` per branch). If the branches are not tracked locally,
`gh stack init <bottom> … <top>` adopts the existing branches without pushing.

Order the chain bottom → top (`main ← PR_1 ← PR_2 ← … ← PR_N`) with each PR's
draft/ready state and the strategy it gets, and show it as a numbered list
**before** any mutation; the user may have meant another top.

| Condition                       | Action                                            |
|---------------------------------|---------------------------------------------------|
| A PR has `state != OPEN`        | Abort; name the closed/merged PR.                 |
| A ready PR sits above a draft   | Abort (Step 0).                                   |
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

## Step 3 — Update chain, bottom-up

```bash
git fetch origin
```

For each PR, bottom first:

**3a. Switch.** `git checkout <head-ref>`. If it fails (dirty tree, divergent local copy), stop and surface the error. Do not `git stash` mid-loop without asking.

**3b. Target.** The bottom branch updates onto the default branch; every other branch onto the previous branch in the stack. Always use the `origin/<target>` ref, so the parent pushed in 3e is the source of truth.

**3c. Update.** Draft: `git rebase origin/<target>`. Ready: `git merge origin/<target> --no-edit`, then confirm `git diff --name-only --diff-filter=U` prints nothing.

Do not pipe either; the pipe hides the exit code. On conflicts, **stop the whole loop** and show:

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

**3e. Push.** Draft: `git push --force-with-lease`. Ready: `git push`.

Never bare `--force`: the lease refuses to overwrite commits that landed on
origin since your last fetch (CI bots, reviewers, another machine). If the push
is rejected, stop and tell the user origin moved; never retry with `--force`.
On success, the next branch rebases onto this one's fresh `origin/` ref.

**With `gh stack`:**

- Ready branches: merge and push as above; `gh stack` has no merge-update mode.
- Drafts above them: `gh stack rebase --no-trunk` rebases each branch onto the one below it and never touches the bottom. Plain `gh stack rebase` (or `gh stack sync` when trunk moved) also rebases the bottom onto trunk, which force-rewrites a ready PR: only use them when every branch is a draft.
- `gh stack` refuses to run when the worktree has any untracked file ("uncommitted changes in worktree"). Do not stash files the user keeps on purpose; run the equivalent `git rebase origin/<parent>` per draft instead. The stack metadata follows Git ancestry, so `gh stack view --json` still reports `needsRebase: false` afterwards.
- Push drafts with `gh stack push` (per-branch `--force-with-lease`) or `git push --force-with-lease`.
- Never `gh stack submit` here: a restack creates no PR, and `submit` writes PR titles and bodies.

## Step 4 — Final status report

```bash
for pr in <numbers...>; do
  gh pr view "$pr" --json number,title,headRefName,baseRefName,isDraft,mergeable,statusCheckRollup,url
done
```

Print one summary:

```text
Stack restacked successfully:

  #N1  <owner>/feat/.../bottom        base: main                       MERGEABLE  ready (merged main)
  #N2  <owner>/feat/.../middle        base: <owner>/feat/.../bottom    MERGEABLE  draft
  #N3  <owner>/feat/.../top           base: <owner>/feat/.../middle    MERGEABLE  draft

CI runs are queued on origin — re-check in a few minutes.
```

Pop the Step 0 stash and tell the user. Flag any PR that is `CONFLICTING`: a
child usually resolved a conflict against an old parent version the rebase did
not preserve.
