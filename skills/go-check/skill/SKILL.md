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

Two-step check for Go code: **lint → test**. Scope to the affected package(s),
not the whole module, unless you're doing a pre-merge full sweep.

## Setup — match CI's golangci-lint version

If the repo pins an exact golangci-lint version in CI (commonly
`.github/workflows/*.yml` via `golangci-lint-action`), whatever's on `$PATH` may
be older and silently skip rules CI enforces (godot's capital/period checks,
revive's `redundant-test-main-exit`, etc.). Install the pinned version once so
direct `golangci-lint` calls reproduce CI:

```bash
# Find the pinned version (adjust the workflow filename to the repo):
grep -rA2 "golangci-lint-action" .github/workflows/ | grep version:
# example output: "          version: v2.11.4"

# Install (writes to $GOPATH/bin, overrides any older binary on $PATH):
go install github.com/golangci/golangci-lint/v2/cmd/golangci-lint@v2.11.4
golangci-lint --version   # confirm
```

**Re-run the install whenever the workflow's pinned version bumps** — `go install`
does not auto-update. One-liner that always pulls whatever the workflow says today:

```bash
v=$(grep -rhA2 "golangci-lint-action" .github/workflows/ | sed -n 's/.*version: //p' | tr -d ' ' | head -1)
go install github.com/golangci/golangci-lint/v2/cmd/golangci-lint@"$v"
```

If a global install is undesirable (locked-down env),
`go run github.com/golangci/golangci-lint/v2/cmd/golangci-lint@<version> run <paths>`
works per-invocation — slower, cached after first fetch.

If the repo doesn't pin a version, just use the `golangci-lint` on `$PATH`.

## Quick check (default — pre-push)

```bash
golangci-lint run ./path/to/changed/pkg/...
go test         ./path/to/changed/pkg/...
```

**Scope to packages you touched** (not `./...`). Pre-push checks are about your
own diff; whole-repo coverage is for the broader full-pipeline path below.

If lint fails, fix the reported issues and re-run lint before running tests.
If tests fail, fix and re-run both steps from the top.

### Don't trust a bare `make lint`

A repo's `make lint` is often just `golangci-lint run` with no path → it walks
the whole module AND uses whatever `$PATH` provides. If you've followed the
Setup step the version is right, but the scope is still whole-repo. For a
pre-push check, stick with explicit package paths.

## Full pipeline (broad changes, pre-merge)

When you actually need whole-repo coverage (e.g. a wide refactor before merging
a long-running branch):

```bash
golangci-lint run ./...
go test -race ./...
# If the repo has integration/nightly build tags, include them as CI does:
# go test -race -tags=integration,nightly ./...
```

If the repo provides faster make targets (`make test-incremental`, `make test`),
prefer them once you've confirmed what they actually run.

## Common failures

| Symptom | Fix |
|---------|-----|
| `errcheck`: unchecked error | Handle or explicitly ignore the return |
| `shadow`: "err" shadows outer | Rename inner var or use `=` instead of `:=` |
| `revive`: missing godoc | Add a comment to the exported symbol |
| `unused` | Remove the dead identifier |
| `FAIL` + diff | Logic regression — re-read the test expectation |
| `build failed` before tests | Compilation error; fix first |

Never edit generated files (`proto/gen/`, `*_gen.go`, `mock_*.go`). Fix the
generator source instead.

## Codacy / Semgrep false positives (CI-only, post-push)

Local `golangci-lint` does not run Codacy/Semgrep — those checks fire only after
push. Codacy runs Semgrep under the hood, so a Codacy `action_required` finding
with a prose message ("The application dynamically constructs file or path
information.", "Detected directly writing … in `http.ResponseWriter.write()`.
This bypasses HTML escaping…") is a Semgrep rule match.

The only form that suppresses **and** still satisfies a godot-enabled lint is an
inline `nosemgrep` directive naming the exact rule id:

```go
offending_line() // nosemgrep: <exact-rule-id>
```

Rules:
- **Inline at the end of the offending line.** A comment on the line above is
  silently ignored.
- **Use the literal `id:` field** from the upstream Semgrep YAML rule. Codacy
  parses the text after the colon as a rule id; freeform prose is interpreted as
  a (non-matching) rule id and the suppression no-ops.
- **godot is fine** with this form — single-identifier comments are treated as
  directives and skipped, even with `period: true`.

### Finding the rule id (in this order)

1. **Grep the same repo first** — most rules are already suppressed somewhere:
   ```bash
   grep -rn "nosemgrep:" .
   ```
2. **Search GitHub semgrep-rules forks for the exact message text:**
   ```bash
   gh api -X GET search/code --field q='"<exact codacy message>" extension:yaml'
   ```
   Open the matched YAML and copy the `id:` field **verbatim** (often namespaced,
   e.g. `go_filesystem_rule-fileread`, not the short tail `rule-fileread`).

### Commonly-seen rule ids (Go)

| Pattern | Rule id |
|---|---|
| `w.Write(...)` direct ResponseWriter write | `no-direct-write-to-responsewriter` |
| `os.ReadFile(filepath.Join(...))` dynamic file path | `go_filesystem_rule-fileread` |
| `import "math/rand"` / `"math/rand/v2"` | `math-random-used` |
| `import "text/template"` | `import-text-template` |

### Forms that DON'T work (don't waste a CI cycle on these)

- `// #nosec G304` / `// nolint:gosec` — wrong tool, Codacy ignores.
- `// nosemgrep -- prose` — suppresses Codacy but trips godot.
- `// nosemgrep: Capitalized prose.` — passes godot, but Codacy reads
  "Capitalized" as a rule id, no match → no suppression.
- `// nosemgrep: <rule-id>` placed on the line above — Codacy ignores.
- Bare `// nosemgrep` — works in repos *without* godot, but trips godot's
  period/capital checks where it's enabled.

When the offending code is *whole-repo test infrastructure* (an entire helper
package living in non-`_test.go` files), prefer adding the path to
`.codacy.yaml` `exclude_paths` over per-line comments — discuss with the
reviewer first; the inline comment is the safer default.

## Done

- All passed → **"Clean."**
- Had to fix something → briefly say what and what changed.
