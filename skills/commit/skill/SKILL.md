---
name: commit
description: Safe git commit helper that enforces a push-safety rule: never amend a commit that already exists on origin. Use this skill whenever the user wants to commit changes, amend a commit, or asks to "commit", "/commit", "create a commit", or "save changes". It handles staging, decides amend-vs-new based on remote state, and runs a quick sanity check.
---

# commit

The rule: **if the tip commit is on origin, make a new commit. If it is not,
amending is fine.**

## Step 1 — Check remote state

```bash
git fetch --quiet
git branch -r --contains HEAD
```

Any output means the tip is pushed: **do not amend**. No output means it is
unpushed: amending is allowed.

## Step 2 — Stage files

Stage what the user named. Otherwise read `git status` and stage the files that
clearly belong to the change. Skip unrelated files and anything that could hold
secrets. Prefer `git add <files>` over `git add -A` or `git add .`.

**After `git reset --soft`** (e.g. to squash), re-stage first. A soft reset moves
`HEAD` but leaves the index at the stale pre-reset snapshot, so later edits are
not staged. The tell is `AM` or `MM` in `git status`. Re-add, then confirm nothing
is left unstaged:

```bash
git add <files>
git diff --stat -- <files>   # empty once everything is staged
```

## Step 3 — Commit

Message: lead with the type (`feat`, `fix`, `refactor`, `chore`, `test`, `docs`)
and scope; one short imperative sentence saying *what* and *why*; match
`git log --oneline -10`.

| Tip state | Command                                                                  |
|-----------|--------------------------------------------------------------------------|
| Unpushed  | `git commit --amend --no-edit` (or `--amend -m "..."` for a new message) |
| On origin | New commit, as below                                                     |

```bash
git commit -m "$(cat <<'EOF'
<message>

Co-Authored-By: Claude <model-name> <noreply@anthropic.com>
EOF
)"
```

## Step 4 — Sanity check

Run a quick check scoped to the affected packages, then report the result. If
tests fail, say so; do not proceed silently.

| Repo                                               | Check                                                                                                   |
|----------------------------------------------------|---------------------------------------------------------------------------------------------------------|
| A stack check skill is installed (e.g. `go-check`) | Use it; it knows the exact lint/test invocation and CI parity.                                          |
| Go (`go.mod`)                                      | `golangci-lint run` and `go test` on the changed packages.                                              |
| Other                                              | The project's standard test command, from `Makefile`, `package.json`, `Cargo.toml`, `pyproject.toml`, … |
| No test infrastructure                             | Skip and say so.                                                                                        |
