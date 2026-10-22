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
  stack", "the stack is out of date". Ready PRs are never force-pushed. Drives
  `gh stack` in repos that require GitHub Stacked PRs. Do NOT use for a single PR against
  main (use /pr-sync). Do NOT use to create PRs (/pr) or commit (/commit).
---

# pr-restack

Re-align a stack bottom-up so each child PR shows only its own commits. The
strategy is chosen **per branch**, from that branch's own PR state:

| Branch's PR      | Update (3c)                                           | Push (3e)                     |
|------------------|-------------------------------------------------------|-------------------------------|
| Draft            | `git rebase origin/<target>`                          | `git push --force-with-lease` |
| Ready for review | `git merge --no-edit origin/<target>` (as `/pr-sync`) | `git push`                    |

The target is the default branch for the bottom and the parent's tip for every
other branch. A draft above a ready parent is still rebased, onto the parent's
new tip. Merging into a draft would inflate every child's diff and duplicate history.

## Step 0 — Pre-conditions

Abort and surface the reason if any fails, before rebasing anything.

1. **Ready PRs sit below drafts.** Rebasing a draft rewrites its commits, so a ready PR above it would need a force-push too. Stop and ask.
2. **Clean working tree.** Tracked changes: `git stash push` (no `-u`) and pop on every exit path; a checkout over uncommitted changes can abort silently, leaving the loop on the wrong branch. Untracked files the user keeps on purpose do not block `git`; leave them.
3. **Default branch known:** `gh repo view --json defaultBranchRef --jq .defaultBranchRef.name`.
4. **Stack tool.** If the repo's `CONTRIBUTING.md` requires GitHub Stacked PRs (`gh stack`), load the `gh-stack` skill and apply [gh stack mode](#gh-stack-mode). Otherwise use plain `git` and `gh pr`.

## Step 1 — Discover the stack

Start from the **top**: the branch the user named, else `git branch --show-current`.
Follow `baseRefName` down until it equals the default branch:

```bash
gh pr view <branch-or-number> --json number,headRefName,baseRefName,isDraft,mergeable,state
```

Order the chain bottom → top (`main ← PR_1 ← … ← PR_N`) with each PR's
draft/ready state. Show it as a numbered list **before** any mutation; the user
may have meant another top.

| Condition                       | Action                                            |
|---------------------------------|---------------------------------------------------|
| A PR has `state != OPEN`        | Abort; name the closed/merged PR.                 |
| A ready PR sits above a draft   | Abort (Step 0.1).                                 |
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

**3b. Target.** Use the `origin/<target>` ref, so the parent pushed in 3e is the source of truth.

**3c. Update.** Per the strategy table. Do not pipe the command; the pipe hides the exit code. A non-zero exit means conflicts: **stop the whole loop** and show:

1. Conflicted files: `git diff --name-only --diff-filter=U`
2. Auto-merged but staged files: `git status --short | awk '/^M /'`
3. About 20 lines of `git diff` around each conflict marker.

If the conflicts are duplicates of commits the new base already contains (many
near-identical files, mostly staged `M ` rather than `UU`), take the base's
version for **every** listed file, staged-`M` ones included. The base is `HEAD`
during a rebase and `MERGE_HEAD` during a merge:

```bash
git status --short | awk '/^[MADRUC][MADRUC ]/ {print $2}' | xargs git checkout <HEAD|MERGE_HEAD> --
```

This discards the branch's version, so use it only when every file is a duplicate.
Never auto-resolve otherwise. Do not abort (`git rebase --abort`, `git merge --abort`) unless asked. The user
resolves, then re-invokes `/pr-restack`; Step 1 finds the resume point.

**3d. Build before pushing.** A clean update does not mean the code compiles:
updating past an API change breaks the build with no conflict signal. Build what
the branch touches relative to `origin/<target>`:

- If a stack check skill is installed (e.g. `go-check`), use it on the touched packages.
- Go without one: run the changed-package build from `/pr-sync` Step 5 with `origin/<target>` as the base. In short, `go build` each changed dir that has no `//go:build` file, and vet tagged dirs with `go vet -tags <tag>`.
- Other stacks: the repo's equivalent compile check. If none is fast, note that no local build ran for this branch.

If the build fails, stop the loop, show the error and ask the user to fix on this
branch and re-invoke. Never push a branch that compiled before the update but not after.

**3e. Push.** Per the strategy table. Never bare `--force`: the lease refuses to overwrite commits that landed on
origin since your last fetch (CI bots, reviewers, another machine). If the push
is rejected, stop and tell the user origin moved; never retry with `--force`.

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

## gh stack mode

Steps 0-4 apply unchanged; only these commands differ.

- Read the chain with `gh stack view --json` (bottom first, `needsRebase` per branch). If branches are not tracked, `gh stack init <bottom> … <top>` adopts them without pushing; it enables `git rerere`.
- Mixed stack: update and push per branch with `git`, as in 3c and 3e. `gh stack` has no merge mode, and its `rebase` and `push` act on every branch, ready ones included.
- All-draft stack: `gh stack rebase` and `gh stack push` may replace 3c and 3e. Add `--no-trunk` to `rebase` unless the default branch moved.
- `gh stack` may refuse to run when the worktree has an untracked file ("uncommitted changes in worktree"). Do not stash files the user keeps; use `git rebase origin/<parent>` per draft instead.
- Never run `gh stack submit` or `gh stack sync`. `submit` creates PRs, updates PR bases and, with `--open`, marks existing PRs ready. `sync` pushes every branch.
