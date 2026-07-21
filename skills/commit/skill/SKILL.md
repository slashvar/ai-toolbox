---
name: commit
description: Safe git commit helper that enforces a push-safety rule: never amend a commit that already exists on origin. Use this skill whenever the user wants to commit changes, amend a commit, or asks to "commit", "/commit", "create a commit", or "save changes". It handles staging, decides amend-vs-new based on remote state, and runs a quick sanity check.
---

# commit

Safe git commit skill. The core rule: **if the tip commit is already on origin, create a new commit. If it hasn't been pushed yet, amending is fine.**

## Step 1 — Check remote state

```bash
git fetch --quiet
git branch -r --contains HEAD
```

If the output is non-empty (the tip commit appears on any remote branch), the commit has been pushed — **do not amend, create a new commit**.

If the output is empty, the tip hasn't been pushed yet — **amending is allowed**.

## Step 2 — Stage files

If the user specified files, stage only those. Otherwise, review `git status` and stage the files that are clearly part of the change being committed. Avoid staging unrelated files or files that look like they could contain secrets.

Prefer `git add <specific-files>` over `git add -A` or `git add .`.

**If you just ran `git reset --soft` (e.g. to squash commits), re-stage first.**
A soft reset moves `HEAD` but leaves the index holding the *stale* pre-reset
snapshot, so working-tree edits made after the reset aren't staged. The tell is
`AM` (or `MM`) in `git status` — the staged copy differs from the working tree.
Re-`git add` the affected files, then confirm nothing is left unstaged before
committing:

```bash
git add <files>
git diff --stat -- <files>   # should be empty once everything is staged
```

## Step 3 — Commit

**If tip is NOT on origin (amend is safe):**
```bash
git commit --amend --no-edit
# or, if a new message is appropriate:
git commit --amend -m "..."
```

**If tip IS on origin (must not amend):**
```bash
git commit -m "$(cat <<'EOF'
<message>

Co-Authored-By: Claude <model-name> <noreply@anthropic.com>
EOF
)"
```

For the commit message:
- Lead with the type (`feat`, `fix`, `refactor`, `chore`, `test`, `docs`) and scope
- One short imperative sentence describing *what* changed and *why*
- Follow the style of recent commits in the repo (`git log --oneline -10`)

## Step 4 — Sanity check

After committing, run a quick check scoped to the affected package(s). Pick the
strategy that matches the repo:

- **A project-specific check skill is installed** (e.g. a `go-check` or
  language/repo quality-pipeline skill): prefer it — it knows the repo's exact
  lint/test invocation and CI parity.
- **Go project** (`go.mod` or `*.go` files): `golangci-lint run ./...` then
  `go test ./...`, scoped to the affected directory.
- **Other languages**: run the project's standard test command if obvious from
  `Makefile`, `package.json`, `Cargo.toml`, `pyproject.toml`, etc.
- **No test infrastructure visible**: skip and note it.

Report the result. If tests fail, say so — don't silently proceed.
