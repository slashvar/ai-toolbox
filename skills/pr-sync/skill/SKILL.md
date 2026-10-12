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

Merge the default branch into the current PR branch, then report PR status.
Merging never rewrites history, so no force-push is needed once reviewers or CI
have seen the branch.

## Step 1 — Identify context

```bash
git branch --show-current
gh repo view --json defaultBranchRef --jq .defaultBranchRef.name
```

If the current branch is the default branch, stop: this skill is for PR branches.

## Step 2 — Check PR state

Fetch state and checks in one call; Step 4 reuses the checks.

```bash
gh pr view --json state,statusCheckRollup
```

| State               | Action                                                                                            |
|---------------------|---------------------------------------------------------------------------------------------------|
| `OPEN`              | Continue to Step 3.                                                                               |
| `MERGED` / `CLOSED` | Skip to Step 6. Tell the user; for `MERGED`, suggest `git checkout <default-branch> && git pull`. |
| No PR               | Tell the user and suggest `/pr`.                                                                  |

This guard stops you merging the default branch into a PR that already landed.

## Step 3 — Fetch and check if sync is needed

```bash
git fetch origin <default-branch>
git merge-base --is-ancestor origin/<default-branch> HEAD && echo UP_TO_DATE || echo NEEDS_MERGE
```

If `UP_TO_DATE`, skip to Step 6 and say so.

## Step 4 — Surface CI status before merging

A merge commit buries a pre-existing failure: the next run cannot show whether
the failure is the branch's or came from the default branch. From the Step 2
`statusCheckRollup`, select checks whose `conclusion` is `FAILURE`, `CANCELLED`,
`TIMED_OUT` or `STARTUP_FAILURE`.

If any exist, list them (name + `detailsUrl`/`targetUrl`) and ask whether to
merge anyway or fix against the un-merged baseline first. Wait for the answer.
In-progress or queued checks are not blockers; mention them and continue.

## Step 5 — Merge, build, push

```bash
git merge origin/<default-branch> --no-edit
```

Never pipe or wrap the merge (`| tail`, `| tee`): the pipe hides a non-zero exit,
and a later "build OK" then runs on a conflicted tree. Confirm the merge is clean
before building:

```bash
git diff --name-only --diff-filter=U    # must print nothing
```

**Conflicts**: stop. Show the conflicting files and their markers, and tell the
user to resolve them and re-run `/pr-sync`. Do not auto-resolve. Do not
`git merge --abort` unless asked; the user may want to resolve in place.

**Clean merge**: a clean merge does not mean the code compiles. A changed
signature, a renamed symbol or a moved package merges without a conflict signal,
and pushing it hands reviewers and CI a broken branch. Build what the branch
touches first:

- If a check skill for the stack is installed (e.g. `go-check`), use it on the
  touched packages.
- Otherwise build the changed packages directly. Go:

  ```bash
  # Dirs of changed files that carry no //go:build constraint.
  git diff --name-only --diff-filter=d origin/<default-branch>...HEAD -- '*.go' |
    while read -r f; do grep -q '^//go:build' "$f" || dirname "$f"; done |
    sort -u | sed 's|^|./|' | xargs -r go build
  ```

  Files behind a build tag (e.g. `e2e`) are skipped, because a dir whose files
  are all excluded fails `go build`. List them (`grep -l '^//go:build'` over the
  changed files) and vet each tagged dir with `go vet -tags <tag> ./dir`.
- Other stacks: use the repo's equivalent compile check. If none is fast, say in
  the summary that no local build verification ran.

If the build passes, `git push`. If it fails, stop and show the error. The merge
is local and unpushed: the user fixes the break, commits and pushes (or re-runs
`/pr-sync`, which will skip to Step 6).

## Step 6 — Show PR status

```bash
gh pr view --json number,title,state,mergeable,statusCheckRollup,reviews,url
```

Summarize: number, title, URL; mergeable (`MERGEABLE` / `CONFLICTING` /
`UNKNOWN`); checks (name + passed/failed/in-progress); reviews (author +
`APPROVED` / `CHANGES_REQUESTED` / `COMMENTED`). With no PR, say so and suggest
`/pr`.
