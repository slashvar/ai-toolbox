---
name: go-check
description: >
  Run the Go quality pipeline (lint → test) for any Go repository. Always use
  this skill whenever the user wants to verify their Go code is correct —
  including after edits, before committing, before opening a PR, or when asking
  if tests pass, if code builds, or if linting is clean. Covers all
  quality-checking requests in a Go codebase: "check my changes", "verify
  everything passes", "does it build", "do the tests pass", "run the pipeline",
  "run a quick check", "format and lint", "will CI be happy", "is the code ok to
  commit", or any request to run go test / golangci-lint / gofmt. Do not use for
  requests to write tests, explain tools, install dependencies, refactor code,
  or debug CI pipelines externally.
---

# go-check

Two steps: **lint → test**, scoped to the packages the diff touches. Go
whole-repo only for a pre-merge sweep.

## Setup — match CI's golangci-lint version

A stale `golangci-lint` on `$PATH` silently skips rules CI enforces (godot's
capital/period checks, revive's `redundant-test-main-exit`, …). If CI pins a
version, install that one. Re-run after every pin bump; `go install` does not
auto-update.

```bash
v=$(grep -rhA2 "golangci-lint-action" .github/workflows/ | sed -n 's/.*version: //p' | tr -d ' "' | head -1)
echo "pinned golangci-lint: $v"   # sanity-check: first `version:` within 2 lines of the action
case "$v" in
  v2*) mod=github.com/golangci/golangci-lint/v2/cmd/golangci-lint ;;
  *)   mod=github.com/golangci/golangci-lint/cmd/golangci-lint ;;   # v1.x: no /v2
esac
go install "$mod@$v" && golangci-lint --version
```

Where a global install is undesirable, `go run "$mod@$v" run <paths>` works per
call (slower; cached after the first fetch). With no pin, use the binary on
`$PATH`.

## Quick check (default, pre-push)

Derive the changed packages once, then lint and test them:

```bash
base=origin/<default-branch>
pkgs() { git diff --name-only --diff-filter=d "$base"...HEAD -- '*.go' | xargs -n1 dirname | sort -u | sed 's|^|./|'; }
pkgs | xargs golangci-lint run --new-from-rev="$base"
pkgs | xargs go test
```

- **Scope**: never lint a parent dir or `./...` pre-push. It surfaces old issues in packages the diff never touched.
- **`--new-from-rev`**: gates only on issues the diff introduces. To see the rest of the changed packages, re-run without it and report those findings **separately** as pre-existing. Do not fix them unasked.
- **Build tags**: a changed dir whose files all sit behind `//go:build <tag>` fails with "build constraints exclude all Go files". Lint it with `--build-tags <tag>` and test it with `-tags <tag>` (e2e tests usually need infra: see below), or drop it from the list.
- **Bare `make lint`**: usually `golangci-lint run` with no path, so it walks the whole module. Pass explicit packages instead.

If lint fails, fix and re-run lint before testing. If tests fail, fix and re-run both from the top.

## Full pipeline (broad changes, pre-merge)

```bash
golangci-lint run ./...
go test -race ./...    # add -tags=<tags> when CI builds integration/nightly tags
```

If the repo has faster targets (`make test-incremental`, `make test`), prefer
them once you have confirmed what they run.

## Common failures

| Symptom                                          | Fix                                                             |
|--------------------------------------------------|-----------------------------------------------------------------|
| `errcheck`: unchecked error                      | Handle or explicitly ignore the return                          |
| `shadow`: "err" shadows outer                    | Rename inner var or use `=` instead of `:=`                     |
| `revive`: missing godoc                          | Add a comment to the exported symbol                            |
| `unused`                                         | Remove the dead identifier                                      |
| `FAIL` + diff                                    | Logic regression — re-read the test expectation                 |
| `build failed` before tests                      | Compilation error; fix first                                    |
| `connection refused`, unset `*_URL`, dial errors | Infra-bound test with no backing service — **not a regression** |

Never edit generated files (`proto/gen/`, `*_gen.go`, `mock_*.go`). Fix the
generator source.

### Infra-bound tests

Tests needing a live service (Postgres, Redis, a broker) fail when it is down
(Docker off), whatever your change. Do not "fix" them and do not gate on them;
CI provisions the infra. Investigate only if the diff touches that integration
path. Skip them locally, most specific first:

| Option                                      | Command                                                    |           |                                  |
|---------------------------------------------|------------------------------------------------------------|-----------|----------------------------------|
| The repo's unit-only target, when it exists | `make test-unit` (check `Makefile` for the name)           |           |                                  |
| By test name                                | `go test -skip 'Redis\                                     | Postgres\ | Integration' <pkgs>` (Go ≥ 1.20) |
| By convention                               | `go test -short <pkgs>`, or omit the integration build tag |           |                                  |

Say in the report which tests were skipped.

### Shell notes

zsh with `noclobber` makes `>` fail silently on an existing file. Snippets that
save output (lint logs, test results) use `>|`.

## Codacy / Semgrep false positives (CI-only, post-push)

Local `golangci-lint` does not run Codacy or Semgrep; they fire only after push.
Codacy runs Semgrep underneath, so a Codacy `action_required` finding with a
prose message ("The application dynamically constructs file or path
information.", "…This bypasses HTML escaping…") is a Semgrep rule match.

Suppress with an inline `nosemgrep` directive naming the exact rule id. It is
the only form that both suppresses and satisfies a godot-enabled lint:

```go
offending_line() // nosemgrep: <exact-rule-id>
```

| Rule                                                 | Why                                                                                                    |
|------------------------------------------------------|--------------------------------------------------------------------------------------------------------|
| Inline, at the end of the offending line             | A comment on the line above is ignored.                                                                |
| Use the literal `id:` from the upstream Semgrep YAML | Codacy reads the text after the colon as a rule id; prose is a non-matching id and suppresses nothing. |
| godot is fine with it                                | A single-identifier comment is treated as a directive and skipped, even with `period: true`.           |

### Finding the rule id

1. Grep the repo first. Most rules are already suppressed somewhere: `grep -rn "nosemgrep:" .`
2. Search GitHub for the exact Codacy message, then copy the `id:` **verbatim**. It is often namespaced (`go_filesystem_rule-fileread`, not `rule-fileread`).

   ```bash
   gh api -X GET search/code --field q='"<exact codacy message>" extension:yaml'
   ```

### Commonly seen ids (Go)

| Pattern                                             | Rule id                             |
|-----------------------------------------------------|-------------------------------------|
| `w.Write(...)` direct ResponseWriter write          | `no-direct-write-to-responsewriter` |
| `os.ReadFile(filepath.Join(...))` dynamic file path | `go_filesystem_rule-fileread`       |
| `import "math/rand"` / `"math/rand/v2"`             | `math-random-used`                  |
| `import "text/template"`                            | `import-text-template`              |

### Forms that do not work

Do not spend a CI cycle on these:

| Form                                        | Why it fails                                                           |
|---------------------------------------------|------------------------------------------------------------------------|
| `// #nosec G304`, `// nolint:gosec`         | Wrong tool; Codacy ignores them.                                       |
| `// nosemgrep -- prose`                     | Suppresses Codacy but trips godot.                                     |
| `// nosemgrep: Capitalized prose.`          | Passes godot, but Codacy reads "Capitalized" as a rule id; no match.   |
| `// nosemgrep: <rule-id>` on the line above | Codacy ignores it.                                                     |
| Bare `// nosemgrep`                         | Fine without godot; trips godot's period/capital checks where enabled. |

When the flagged code is a whole helper package of test infrastructure living in
non-`_test.go` files, `.codacy.yaml` `exclude_paths` beats per-line comments.
Discuss with the reviewer first; the inline comment is the safer default.

## Done

All passed → **"Clean."** Otherwise say briefly what you fixed, what was skipped
(infra-bound tests) and any pre-existing findings outside the diff.
