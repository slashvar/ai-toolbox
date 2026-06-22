---
name: pr-sync
description: >
  Keep a PR branch up to date with the default branch using merge (not rebase)
  to avoid force-pushes. Use this skill whenever the user wants to sync their
  PR with main, update their branch, pull in latest changes from main/master,
  check PR status, or says things like "sync my PR", "update branch",
  "merge main into my branch", "is my PR up to date", "/pr-sync",
  "check PR status". Also use when the user asks to resolve a
  "branch is out of date" or "conflicting" mergeable status on a PR.
  Do NOT use for creating PRs (use /pr), committing (use /commit), or
  rebasing (the user must explicitly ask for rebase outside this skill).
---

# pr-sync

Sync the current PR branch with the default branch using merge, then report
PR status. The merge strategy avoids force-pushes, which is important once a
branch has been shared with reviewers or CI.

## Step 1 — Identify context

Run these in parallel:

```bash
git branch --show-current
git remote show origin | grep 'HEAD branch'
git log --oneline -1
```

From these, determine:
- **current branch**: the branch you're on
- **default branch**: whatever `HEAD branch` reports (typically `main` or `master`)

If the current branch IS the default branch, stop and tell the user — this
skill is for PR branches, not the default branch.

## Step 2 — Check PR state

Before any git operations, verify the PR is still open:

```bash
gh pr view --json state --jq .state
```

- **OPEN**: proceed to Step 3.
- **MERGED**: skip to Step 6 (status). Tell the user the PR is already merged —
  no sync needed. Suggest `git checkout <default-branch> && git pull` to get
  back on the default branch.
- **CLOSED**: skip to Step 6. Tell the user the PR is closed.
- **No PR found**: tell the user no PR exists for this branch and suggest `/pr`.

This guard prevents accidentally merging main into a branch whose PR has already
landed.

## Step 3 — Fetch and check if sync is needed

```bash
git fetch origin <default-branch>
git merge-base --is-ancestor origin/<default-branch> HEAD && echo "UP_TO_DATE" || echo "NEEDS_MERGE"
```

If `UP_TO_DATE`: skip to Step 6 (status check). Tell the user the branch is
already up to date with `origin/<default-branch>`.

## Step 4 — Surface CI status before merging

Before changing the branch, check whether the current head commit has failing
CI checks. Burying a pre-existing failure under a merge commit muddies the
debugging trail — the user can't tell from the next run whether the failure
is theirs or came from `main`.

```bash
gh pr view --json statusCheckRollup \
  --jq '[.statusCheckRollup[]? | select(.conclusion == "FAILURE" or .conclusion == "CANCELLED" or .conclusion == "TIMED_OUT" or .conclusion == "STARTUP_FAILURE")
         | {name, conclusion, url: (.detailsUrl // .targetUrl)}]'
```

If the array is non-empty:

1. List the failing checks (name + URL) to the user.
2. Ask whether to proceed with the merge anyway, or stop so the user can fix
   the failure first against the un-merged baseline.
3. Do NOT proceed automatically — wait for the user's decision.

In-progress / queued checks are not blockers; mention them in passing if
they're the only signal, but continue.

## Step 5 — Merge (not rebase)

The branch is already pushed to origin, so rebase would rewrite history and
require a force-push. Merge preserves the existing commit history.

```bash
git merge origin/<default-branch> --no-edit
```

**If the merge succeeds**: push immediately.

```bash
git push
```

**If there are merge conflicts**: stop. Show the user:
1. Which files conflict (`git diff --name-only --diff-filter=U`)
2. The conflict markers in each file (read them)
3. Tell the user to resolve conflicts manually, then run `/pr-sync` again

Do NOT attempt to auto-resolve conflicts. Do NOT run `git merge --abort`
unless the user asks — they may want to resolve in place.

## Step 6 — Show PR status

Find the PR for the current branch and display its status:

```bash
gh pr view --json number,title,state,mergeable,statusCheckRollup,reviews,url \
  --jq '{
    number: .number,
    title: .title,
    state: .state,
    mergeable: .mergeable,
    url: .url,
    checks: [.statusCheckRollup[]? | {name: .name, status: .status, conclusion: .conclusion}],
    reviews: [.reviews[]? | {author: .author.login, state: .state}]
  }'
```

Present a clean summary:
- PR number, title, URL
- Mergeable status (MERGEABLE / CONFLICTING / UNKNOWN)
- CI checks: name + status (passed/failed/in-progress)
- Reviews: author + state (APPROVED / CHANGES_REQUESTED / COMMENTED)

If no PR exists for this branch, say so and suggest `/pr` to create one.
