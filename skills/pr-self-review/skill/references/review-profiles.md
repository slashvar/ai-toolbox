# Review profiles

Per-language addenda for the cold-read reviewer brief. The SKILL's Step 2
detects the stack from a marker file and pastes the matching **Focus** and
**Skip** blocks into the brief (Step 3), layered on top of the generic core.

Conventions:
- **Focus** = idioms a cold reviewer *should* check for this stack.
- **Skip** = which tools own the no-go territory for this stack, named per the
  shared skip rule below.

**Shared skip rule (every stack).** Don't raise findings for anything the
configured toolchain already enforces — formatting, import ordering, stylistic
lints, naming conventions, and type errors a type-checker already reports. Each
profile's **Skip** line below just names the tools that own this territory for
that stack; if a project pins a different toolchain, swap in the linter
actually configured in the repo.

---

## Go — marker: `go.mod`

**Focus addendum**
- Error handling: wrapped with `%w` where the caller may need `errors.Is`/`As`; no swallowed errors.
- `context.Context` propagation: passed (not `context.Background()` deep in call stacks), honored for cancellation.
- Goroutine lifetime: every spawned goroutine has a clear exit path; no leaks on error returns.
- Resource cleanup: `defer Close()`/`Unlock()` placed right after acquisition; no double-close.
- Exported API surface: doc comments on exported identifiers; zero-value usability where idiomatic.
- Slice/map aliasing: appends that may mutate a caller's backing array; nil-map writes.

**Skip addendum** — owned by `gofmt`, `go vet`, and `golangci-lint` (incl. errcheck/ineffassign when enabled in CI).

---

## Rust — marker: `Cargo.toml`

**Focus addendum**
- Ownership/borrowing clarity: unnecessary `.clone()`; lifetimes that obscure intent.
- Error handling: `?` vs `.unwrap()`/`.expect()` in non-test code; error types that lose context.
- `Option`/`Result` handling: silent `unwrap_or_default()` that hides bugs.
- `unsafe` blocks: justified, minimal, with a safety comment.
- Public API: `#[must_use]` where dropping the value is a bug; trait bounds not over-constrained.
- Iterator vs manual loop where it materially aids clarity (not style).

**Skip addendum** — owned by `rustfmt` and `clippy`.

---

## JavaScript / TypeScript — marker: `package.json`

**Focus addendum**
- Async correctness: unawaited promises; missing `try/catch` around `await`; floating promises.
- TypeScript types: `any` escapes, unsound casts (`as`), missing null/undefined handling.
- Public/exported function signatures: are the types honest about what can be returned?
- Error propagation: errors swallowed in `.catch(() => {})`; rejections that vanish.
- Resource/listener cleanup: event listeners, timers, subscriptions removed.
- React (if present): missing/incorrect `useEffect` deps, key usage, state-update batching assumptions.

**Skip addendum** — owned by `prettier`, `eslint`, and `tsc` (type errors).

---

## Python — marker: `pyproject.toml` / `setup.py` / `requirements.txt`

**Focus addendum**
- Exception specificity: bare `except:` / `except Exception` that hides bugs; lost tracebacks.
- Type hints on public functions; correctness of `Optional`/union handling.
- Context managers (`with`) for files/locks/connections instead of manual close.
- Mutable default arguments (`def f(x=[])`).
- Generator vs list materialization where memory matters.
- Public API: docstrings on exported functions/classes; keyword-only args where it aids clarity.

**Skip addendum** — owned by `black`, `ruff`/`flake8` (+ `isort`), and `mypy` (type errors, when in CI).

---

## Java / Kotlin — marker: `pom.xml` / `build.gradle`

**Focus addendum**
- Resource handling: try-with-resources for `AutoCloseable`; no leaked streams/connections.
- Null handling: `Optional` misuse; nullable returns without annotation.
- Exception design: checked-vs-unchecked choice; swallowed exceptions; lost causes.
- Concurrency: shared mutable state, missing `synchronized`/`volatile`, executor shutdown.
- Public API: immutability where appropriate; `equals`/`hashCode` consistency.
- Kotlin (if present): nullability (`!!`), coroutine scope/cancellation.

**Skip addendum** — owned by `spotless`/`ktlint`, `checkstyle`, and `detekt`.

---

## Ruby — marker: `Gemfile`

**Focus addendum**
- Exception handling: `rescue` without a class; swallowed exceptions; `rescue => e` that ignores `e`.
- nil safety: `&.` usage, methods returning nil unexpectedly.
- Public API: clear method contracts; keyword args where positional is ambiguous.
- Resource cleanup: blocks for files/connections instead of manual close.
- Metaprogramming clarity: `method_missing` / `define_method` that obscures behavior.

**Skip addendum** — owned by `rubocop` (formatting + stylistic cops).

---

## Generic (no marker matched)

No addendum. The reviewer uses the generic core from the SKILL only. Tell the
user the stack wasn't detected so they know the review is language-neutral.
