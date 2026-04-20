# Corner Case Programming — Learnings

## Exercise 1: `list.c` — `insert_at` (warm-up)

**Before**: 3 exit points, cyclomatic complexity ~4, two redundant guards.
**After**: 1 exit point, cyclomatic complexity 2, no redundant guards.

### Observations

- **Corner-case code can mask bugs.** The `cur` counter was never incremented in the traversal loop (the body was just `continue`). This bug was invisible because Cases 1 (empty list) and 2 (end of list) short-circuited the traversal entirely. Removing the guards made the bug immediately apparent — without them, the loop must be correct for all positions.

- **The existing codebase already had the answer.** `push_front` was already written as the clean two-line splice (`e->next = l->next; l->next = e`). The refactored `insert_at` is just `push_front` with a traversal prefix. When one function in a codebase is clean and another is corner-cased, the clean one often serves as proof that the general logic works.

- **The condition-test rule is a strong mechanical detector.** CLAUDE.md's rule — "between two tests on the same condition, there should exist a code path modifying that condition" — directly flagged `it->next == NULL` being tested at line 30 and again at line 40 with no modification of `it->next` between them (the loop modifies `it`, not its `next` field, and on an empty list the loop doesn't execute at all).

## Exercise 2: `list-tdd` — `insert_at` via red-green-refactor

**Result**: Arrived at the same 7-line, zero-corner-case solution as exercise 1, built from scratch via TDD.

### Observations

- **Generalize, don't guard.** At cycle 2, passing `test_insert_front` required preserving the existing list. The natural move was `e->next = l->next; l->next = e` — which *replaced* the cycle-1 code rather than wrapping it in an `if`. This is the key TDD discipline: when new code subsumes old code, delete the old code instead of guarding around it.

- **Fold boundary checks into loop conditions, not separate `if` blocks.** At cycle 3, the traversal segfaulted because the loop walked past the end. The corner-case temptation was `if (it->next == NULL) { ... } else { ... }` after the loop. Instead, adding `it->next != NULL` to the loop's own condition handled end-of-list, beyond-end, and empty-list simultaneously — passing all 6 tests at once.

- **TDD doesn't inherently produce corner cases.** The anti-pattern comes from treating each failing test as a *special case* to handle. If the green step asks "what's the general rule that covers this test AND the previous ones?" instead of "what `if` do I add for this test?", the code stays clean through the entire cycle.

- **Both workflows converged.** Refactoring existing corner-case code (exercise 1) and building from scratch via TDD (exercise 2) produced identical results. This suggests the clean solution is a stable attractor — you reach it whether you start messy and simplify, or start empty and generalize.

## Exercise 3: `btree` — top-down insertion via TDD

**Result**: 32 tests (20 tool tests + 12 insert tests), all green. Insert function: ~30 lines with rotation, split, duplicate handling, and no corner-case branching.

### TDD cycle observations

- **Cycles 1-3 (leaf only)**: `lower_bound` + `insert` handled empty, single, and full nodes from the start. No "if empty" guard needed — same pattern as list-tdd.

- **Cycle 4 (root split)**: The root-split prologue is a *precondition setup*, not a corner case. It modifies `root` before the descent loop, establishing the loop invariant "current node is not full." The condition-test rule confirms this: state changes between the prologue's `is_full(root)` and the loop's `is_full(cur->children[i])`.

- **Cycle 6 (duplicates)**: The `continue` + re-evaluation approach initially seemed clean for handling promoted medians. But this hid a deeper issue that only surfaced when rotation was added.

- **Cycle 8 (rotation)**: `continue` after rotation caused **infinite oscillation** — rotating a key to a sibling could shift the target key to that sibling, triggering a reverse rotation. Fix: after rotation, recompute the child index and fall through to split if the new target is still full. No `continue` after rotation — only after split.

### Key learnings

- **Not all repeated patterns are corner cases.** The `find_or_index` call appears 3 times in the loop body (initial, post-rotation, post-split). Each call follows a mutation that invalidates the prior result. This is necessary recomputation, not redundant code. Factoring it into a helper improved readability without eliminating the repetition.

- **`continue` can hide state-machine bugs.** Using `continue` to "re-evaluate from the top" after any mutation seemed elegant. It worked for splits (which reduce full-node count, guaranteeing progress). But rotations don't guarantee progress — they can create the symmetric problem. The fix was structural: different control flow for reversible operations (rotation → recompute + descend) vs irreversible ones (split → recompute via `continue`).

- **The sentinel pattern scales.** In list exercises, the sentinel eliminated empty-list corner cases. In B-trees, the root-split prologue serves the same role: it normalizes the tree so the descent loop never encounters a full root. Both are "pay once at entry, simplify the entire loop."

- **Test infrastructure bugs are real.** The `check_invariants` function had the wrong minimum-key threshold (`ceil(K/2)` instead of `K/2` per spec). This caused false failures that looked like insert bugs. Lesson: validate test helpers with dedicated tests before using them to validate production code.

### Post-exercise simplify pass

Running a code quality review (`/simplify`) after the exercise produced mostly code-hygiene fixes (dead code, misleading types, stale comments) — not corner-case findings. Two observations:

- **Helpers born from corner-case elimination can simplify unrelated code.** `find_or_index` was created to deduplicate the `lower_bound` + duplicate-check pattern in `insert`. The simplify pass revealed that `search` had the same pattern inline, and refactoring it to use `find_or_index` also eliminated the dead `find_child_index` helper. Lesson: the abstractions that emerge from cleaning up control flow often turn out to be the right abstractions for the whole module.

- **Tracking booleans are a smell from incomplete generalization.** The `rotated` flag in `insert` was added to work around rotation oscillation — different control flow for reversible vs irreversible operations. This is a pragmatic tradeoff but goes against the "fold conditions into loops, don't add flags" principle from earlier exercises. It signals the rotation integration could be cleaner.

**Takeaway**: `/simplify` operates on a different axis than corner-case elimination (code organization vs control flow structure) but is worth running as a back-check — the occasional overlap can surface insights.

### Post-exercise manual review: postconditions over re-scans

Manual review of the `insert` function revealed that `find_or_index` calls after rotate/split were unnecessary — each transformation has a deterministic, documented effect on exactly one parent key. Replacing O(log K) re-scans with O(1) comparisons against the known changed key eliminated the `rotated` flag, three `find_or_index` calls, and `std::tie` usage.

- **How we got here.** During TDD, `find_or_index` emerged as a clean way to handle the `continue`-after-mutation pattern. When `continue` caused oscillation, we restructured to call `find_or_index` after each transformation. This was correct but opaque — it treated each transformation as a black box and re-derived state from scratch.

- **What we missed.** Our SPEC.md already documented the postconditions (split promotes median to `keys[i]`, rotation moves a specific key to `keys[i-1]` or `keys[i]`). The CLRS comparison even showed the postcondition-based approach (`if key > keys[i]: i++`). We had the contracts but didn't use them at the call site.

- **The broader principle.** Always reason through pre/post conditions and invariants — both of the functions you call and the ones your own code must enforce. After a transformation, restore invariants using the transformation's known contract, not by re-deriving state from scratch. This connects to the precondition prologue pattern: the root split prologue is also about enforcing an invariant (root not full) before the descent loop.

## Exercise 4: `btree` — B-tree key removal via TDD

**Result**: 20 remove tests + 7 helper tests, all green (59 total). `remove` function: 30 lines, 1 exit point, no tracking flags, no re-scans.

### TDD cycle observations

- **5 cycles to complete** (vs 8 for insert). Planning with postconditions and case unification before writing code meant fewer wrong turns and less refactoring.

- **Case unification via predecessor swap.** CLRS has 3 sub-cases for "key in internal node." We collapsed them to 3 lines: `target = find_max(children[idx]); keys[idx] = target; i = idx;` followed by the standard descent. The descent's own fattening machinery handles everything. This is pattern #7: transform the problem so variants collapse into one path.

- **Fattening needs no postcondition adjustments.** During insert, rotation changes parent keys, which can redirect which child the key belongs to — requiring postcondition comparisons. During delete, the target child is already known. Fattening makes it viable without redirecting. Only merge requires `i--` when merging with the left sibling. The structural reason: insert is SEARCHING (position depends on parent keys), delete has FOUND (position is fixed).

- **Insert/remove duality.** Insert has a prologue (split root if full) + descent that pre-splits. Remove has descent that pre-fattens + epilogue (shrink root if empty). Both are "pay once at the boundary, simplify the entire loop." The tree grows at the root and shrinks at the root.

- **Applying guidelines from the start worked.** The postcondition guideline (pattern #6) and the priority chain pattern (#4) were applied at planning time, not discovered during refactoring. Result: no wasted cycles on re-scans or tracking flags. The lessons from insert directly shaped a cleaner remove.

### Key learning

**Guidelines compound.** Each exercise builds on the previous ones. By exercise 4, we had enough patterns to plan the implementation correctly from the start. The TDD cycles became shorter (5 vs 8) and the refactoring steps became trivial. This suggests the skill we're building is approaching usable maturity — the patterns are predictive, not just retrospective.

## Exercise 5: `spanner_clustering` — refactoring real-world code

**Result**: 5 phases of refactoring on a ~23K header-only C++14 library. 9 constructors → 3, 2 dead methods removed, 1 known bug fixed, all 32 existing tests pass.

### Observations

- **Constructor overload chains are a form of corner case programming.** 9 constructors across 3 types encoded 2 independent options via overloads + dummy `bool` params. Each overload was a "corner case" for a specific combination of features. Default parameters collapse them to 1 per type. New pattern #8.

- **"Should not happen" is a code smell with two root causes.** In `parent()`, the fallback return exists because the author wasn't sure the contract was airtight. The right fix was upstream: making `find_heads` robust to leaf nodes (the null-pointer crash). Once the contract is sound, the defensive code becomes either an assertion or dead weight. New pattern #9.

- **Real code has mixed concerns.** Unlike purpose-built exercises, the spanner codebase mixed genuine corner cases (constructor overloads, redundant null assignment in `split_r`) with code quality issues (dead methods, check-then-insert idiom) and actual bugs (null crash, division by zero). Only some of these are corner case programming — the skill needs to distinguish between patterns it should fix and code smells that belong to a different tool (like `/simplify`).

- **The pattern catalog transferred well.** Patterns #1 (redundant boundary guard) and #6 (postcondition) applied directly to `split_r` and `distribute`. The new patterns (#8, #9) emerged from real-world code that purpose-built examples wouldn't have produced.

### Key learning

**Real code validates the catalog but also extends it.** Purpose-built exercises found patterns in control flow (guards, loops, postconditions). Real code added patterns in API design (overload selection) and error handling (defensive fallbacks). A complete skill needs both categories.

## Exercise 6: `queryparam/proto` in metis — large real-world Go, validating Principle C

**Result**: 3 small genuine corner cases fixed. All tests pass (queryparam/* and reducer/*). Full build succeeds. Branch: `marwan/refactor/queryparam-corner-cases`. Diff: 2 files, -14/+11 lines.

### The big finding: most of the package is NOT corner case programming

The package has 4 mega-functions with ~100 branches each:
- `Accept(field, value)` — type-dispatched field setter
- `all(asURL)` — type-dispatched serializer iterator
- `byName(p, name)` — field getter by name
- `Override(x)` — field-by-field merge with nil checks

These LOOK like corner case programming (massive repetition, many branches), but they're not. Each of 100+ parameters is genuinely different (different type, default, serialization). This is the correct way to handle exhaustive field dispatch in Go without reflection or code generation. The repetition is the cost of static type safety.

**This is Principle C in action at scale.** The condition-test rule confirms: between the 100 branches, no condition is tested twice on the same state — each branch handles a distinct field.

### Small genuine corner cases that WERE fixed

1. **`Filters` merge in `Override`** (pattern #7): replaced `f := ""` + conditional reassignment with direct if/else. Cleaner control flow, same behavior.

2. **`mergeExtensions` early return** (pattern #1): removed `if current == nil && len(update) == 0 { return nil }` — the post-loop `if len(current) == 0 { return nil }` already handles this case. Verified by tracing: both paths produce `nil`.

3. **`valueToUInt64`** (principle A): replaced awkward "unconditional assign + conditional retry" with a clean type switch. Also eliminated the odd "return pointer-to-zero on error" behavior.

### Key learnings

- **Principle C discrimination is the skill's most valuable feature on real code.** Without it, a refactorer would try to "DRY up" the 4 mega-functions and either break things or add significant complexity (reflection, codegen). Recognizing that the repetition is essential variation saves the codebase from misguided refactoring.

- **Small corner cases hide inside big functions.** The 3 genuine issues were not in the mega-functions — they were in smaller helpers that the mega-functions call. Lesson: scan by function, not by file. Don't let visual repetition mask the smaller patterns.

- **Some findings are not corner cases but still fixable.** The `new(fmt.Sprintf(...))` in the filters merge wasn't corner case programming — it was just awkward Go syntax that works but confuses readers. The refactor was about clarity, not corner case elimination. Good to note: our skill catches corner cases, but code quality review (`/simplify`) catches other smells.

- **Scope discipline matters.** The temptation was to attempt the mega-function refactor (table-driven, reflection, codegen). We resisted. The 3 small wins delivered real value; the big refactor would have been risky on 40+ callers. Principle: don't attempt changes whose risk exceeds the clarity win.

## Exercise 7: `queryparam/parse` in metis — generics for Pattern #7 in Go

**Result**: 4 changes across 2 files. All tests pass. Branch: same as exercise 6. Net -31 lines.

### The big finding: tests document corner case contracts

`corner_cases_test.go` literally lists the intentional empty-string behaviors — `aroundLatLng=""` → `[0,0]`, `insidePolygon=""` → `[]`, etc. These are the public contract, tested explicitly. The empty-string guards in the decoders implement them. **Removing those guards would be a breaking change disguised as a refactor.**

This is a critical lesson: when a codebase has a file named `corner_cases_test.go`, its contents are a list of **intentional corner cases** — not candidates for elimination but guardrails for the existing behavior.

### Target that mattered: unifying 4 merge functions with generics (Pattern #7)

The four merge functions (`mergeFilters`, `mergeAnalyticsTags`, `mergeDevFeatureFlags`, `mergeArrayFilters`) all had the same 4-step pipeline:
1. Empty-value guard
2. `Fields[key]` lookup + error
3. Setter closure with type assertion
4. Call `parser.ToValueWithSetter`

Only the type and accumulation varied. Unified via a generic helper `mergeTyped[T any](field, value, fromURL, accumulate func(T))`. Each specific function now shows only the field name + what to accumulate — the plumbing is gone.

72 lines → 39 lines. The variation between functions (which was masked by repeated boilerplate) is now explicit and visible.

### Key learning

**Generics are Pattern #7's best tool in Go.** The previous demonstrations of Pattern #7 (B-tree remove's predecessor swap) were about algorithmic transformation. This is about boilerplate transformation. Both forms of the pattern share the same principle: when N paths lead to the same operation with parameter variation, extract the operation and parameterize the variation.

**This exercise reinforces exercise 6**: on real code, most "duplication" is genuine variation (Principle C), but the genuine Pattern #7 cases are worth the refactoring effort. The skill is distinguishing them — and that's exactly what our catalog helps with.
