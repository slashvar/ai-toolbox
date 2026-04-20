# Exercise 6: `queryparam/proto` in metis

## Source

Real-world Go package at `github.com/algolia/metis/modules/pkg/queryparam/proto`. ~2,000 LOC across 5 files. Handles JSON/URL marshaling of query parameters, merging, and field iteration. 40+ external callers, comprehensive test coverage.

## Branch

Working in `marwan/refactor/queryparam-corner-cases`.

## Finding: mostly not corner case programming

The package has extensive repetition — `Accept()`, `all()`, `byName()`, `Override()` each have ~100 branches, one per parameter. These are **Principle C cases** (false positives): each of 100+ parameters is genuinely different. Not corner case programming.

### Principle C classification of the mega-functions

| Function | Branches | Why not corner cases |
|---|---|---|
| `Accept(field, value)` | ~100 cases | Each parameter has a distinct type and conversion. `valueToPointer`, `valueTo`, `arrayFiltersToProto`, etc. Genuine type variation. |
| `all(asURL)` | ~100 cases | Each parameter has a distinct serialization (JSON vs URL-encoded, different wrappers). Genuine variation. |
| `byName(p, name)` | ~100 cases | Each parameter is a distinct field on the struct. No general "get field by name" without reflection. |
| `Override(x)` | ~100 nil-checks | Each parameter is independently overridable. The nil check is not a boundary guard — it's "was this field set in the override?". |

These are the correct way to handle 100+ distinct fields in Go without reflection or code generation. The repetition is the cost of static type safety.

## Genuine targets

### Target 1: `insideBoundingBoxGrouper` / `insidePolygonGrouper` (helper.go:379-424)

**Issue A (nil vs empty inconsistency)**: `nil` input returns `([][]float64{}, nil)` (empty slice), but empty-after-filtering returns `(nil, err)`. The two "no valid boxes" cases have different observable behavior.

**Issue B (redundant post-loop check)**: `if len(r) == 0 { return nil, err }` after the loop duplicates logic. A nil-or-all-empty input passes the nil check, hits the loop which skips all entries, then fails the post-loop check. The post-loop check exists because the nil check doesn't cover all-empty input.

**Pattern**: #1 (redundant boundary guard) — the `if boxes == nil` check could be subsumed by treating nil as "no valid input."

### Target 2: `Filters` merge logic (merge.go:370-376)

```go
if x.Filters != nil && len(*x.Filters) > 0 {
    f := ""
    if p.Filters != nil && len(*p.Filters) > 0 {
        f = fmt.Sprintf("%s AND ", *p.Filters)
    }
    p.Filters = new(fmt.Sprintf("%s%s", f, *x.Filters))
}
```

The `f := ""` followed by conditional reassignment is awkward. Two cleaner forms possible:
- Explicit if/else for the two cases
- Precompute the new value in a variable with clearer naming

**Pattern**: possibly #7 (problem transformation) — transform the inputs so the formatting is uniform.

### Target 3: `mergeExtensions` nil checks (helper.go:29-56)

```go
if current == nil && len(update) == 0 {
    return nil
}
if current == nil {
    current = make(map[string]map[string]any, len(update))
}
```

Two separate nil checks on `current`. The first is an optimization (skip allocation when there's nothing to do). The second establishes an invariant for the rest of the function.

**Pattern**: likely legitimate (the first is a short-circuit, the second is setup). Worth closer reading.

## Workflow

1. Refactor Target 1 — fix the grouper inconsistency and redundant check
2. Refactor Target 2 — clarify the filters merge
3. Review Target 3 — decide if it's actually a corner case
4. Run tests after each change
5. Full `go test ./...` before committing

## Verification

- `go test ./modules/pkg/queryparam/proto/...` after each change
- Full test suite on completion
