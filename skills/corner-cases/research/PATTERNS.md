# Corner Case Programming — Pattern Catalog

## What this is

Corner case programming is an anti-pattern where code is split into too many specific sub-cases that the general case already handles. The patterns below are organized under three principles discovered across 7 exercises (linked list, B-tree insert/remove, real-world C++ and Go refactoring).

## Why it matters

**Corner case code can mask bugs.** Special-case branches bypass the general logic, hiding its flaws. When you eliminate corner cases, latent bugs in the general path become visible and fixable. Exercise 1 exposed a silent `cur` increment bug in linked list traversal that had been hidden by corner-case guards. This is the value of the discipline: cleaner code, plus bugs you didn't know you had.

## How to use this catalog

1. **Scan for smells**. For each pattern, read the first paragraph and look for candidates in the target code.
2. **Classify each candidate** using the detection rules (see "Detection rules" section below).
3. **For true corner cases**: apply the pattern's fix. Run tests.
4. **For Principle C matches** (false positives): document why it looks suspicious but is essential. This is valuable review output.
5. **During TDD specifically**: at each green step, ask "what's the general rule that covers this test AND the previous ones?" not "what `if` do I add for this test?"

---

## Principle A: Write the general case, not the specific one

The general logic, when written correctly, handles boundaries naturally. Corner cases arise when the programmer writes explicit handling for specific inputs instead of recognizing them as instances of the general case.

### 1. Redundant boundary guard

**Smell**: An `if` block near a loop tests the same condition the loop tests. Its body duplicates the general code. Two common variants:

- **Before a loop**: guards the case where the loop executes zero iterations. The post-loop code already handles it.
- **After a loop**: re-tests a loop exit condition. The subsequent general code already covers it.

**Fix**: Delete the guard. Verify by substituting boundary values (empty collection, zero index, null pointer) through the general-case code and confirming identical behavior.

**Example** (`list.c`):
- Before: `if (l->next == NULL) { splice; return; }` before a traversal loop — the splice after the loop already handles the zero-iteration case.
- After: `if (it->next == NULL) { splice; return; }` after the loop — the general splice `e->next = it->next; it->next = e` already sets `e->next` to NULL when `it->next` is NULL.

**Distinguishable from #3 (precondition prologue)**: a prologue *modifies state* that the loop depends on. A boundary guard tests without modifying.

### 7. Transform to unify

**Smell**: Multiple branches each do different setup but fall through to the same general logic. The branches exist because the problem has variants that all lead to the same operation.

**Fix**: Transform the input so all variants collapse into one path. Apply the transformation first, then let the general machinery handle the rest.

**Example** (`btree remove`): CLRS has 3 sub-cases for "key in internal node." Instead of branching, always swap with predecessor (3 lines), then fall through to the standard descent. The descent's fattening logic handles making the child viable.

**Tool mapping**: In Go, C++, Rust, etc., **generics/templates are this pattern's best tool for boilerplate unification**. Exercise 7's `mergeTyped[T any]` in the metis parse package unified four boilerplate-heavy merge functions (72 → 39 lines) while making the per-type variation explicit.

**Distinguishable from #4 (priority chain)**: transform-to-unify has one outcome reached via all paths after a preparatory step. A priority chain has different *preconditions* but the same postcondition — the branches themselves are necessary.

### 8. Overload selection

**Smell**: Multiple constructor/function overloads that differ only in optional features. Dummy parameters (often `bool`) select between overloads. Bodies are near-identical.

**Fix**: Consolidate into a single entry point with default parameters. The caller explicitly names what it wants instead of relying on overload resolution with dummy values.

**Example** (`spanner_clustering`): 9 constructors across 3 types for 2 independent options → consolidated to 3 with default parameters. Dummy `bool` params disappeared.

**Distinguishable from #4 (priority chain)**: overload selection is API-level duplication for feature combinations; a priority chain is algorithm-level branching with mutually exclusive preconditions.

---

## Principle B: Use what you know about state

After any operation, you know more than you think. A function's contract tells you exactly what changed. An invariant established at entry holds throughout the loop. A "should not happen" state means the contract is unclear. Use this knowledge directly instead of re-deriving or working around it.

### 6. Use postconditions, don't re-scan

**Smell**: After a transformation, the caller re-scans the data structure to rediscover the new state — even though the contract specifies exactly what changed.

**Fix**: Use the transformation's known effect directly (O(1) comparison) instead of re-scanning (O(log n) search).

**Example** (`btree insert`): After `split_child(cur, i)`, the median is at `cur->keys[i]`. Compare directly: `if (key == cur->keys[i]) return; if (key > cur->keys[i]) i++;` — instead of calling `find_or_index`.

**Corollary**: when a transformation doesn't invalidate your position (e.g., fattening a child you've already targeted), no adjustment is needed at all. Insert is SEARCHING (parent key changes can redirect); delete has already FOUND (position is fixed).

**Corollary — Tracking flags**: a boolean stored to decide later control flow is often a symptom of missing postconditions. The `rotated` flag in btree insert existed only to decide whether to call `find_or_index` afterward — using postconditions eliminated both the flag and the re-scan. **Not a smell**: flags that cache expensive computations (legitimate memoization) or flags that carry information across genuinely separate phases.

### 9. Defensive code for unreachable states

**Smell**: Return statements or branches with "should not happen" comments handling cases the author believes are impossible.

**Fix**: Diagnose the root cause — one of two patterns applies:
- **Contract is well-established**: the defensive code is dead weight. Convert to an assertion (fail-fast) or remove.
- **Contract is unclear**: the defensive code works around a missing invariant. Fix the invariant upstream — then the state becomes provably unreachable.

**Example** (`spanner_clustering`): `parent()` returns `split_tree.root` with "should not happen." The fix was upstream: making `find_heads` robust to leaf nodes (the null crash that could create orphan nodes). Sound contract → defensive code becomes a provable dead path.

**Distinguishable from #6**: #6 is about using knowledge you HAVE (the contract); #9 is about handling knowledge you DON'T have (unclear contract). The fix for #6 is reading the contract; the fix for #9 is fixing the contract.

---

## Principle C: When it looks redundant but isn't

Two tests on the same condition are only redundant if they test the same state with the same outcome. Three patterns look redundant on the surface but are essential. The **condition-test rule** from CLAUDE.md is the mechanical detector: "between two tests on the same condition, there should exist a code path modifying that condition."

**Heuristic for real-world code**: a test file named `corner_cases_test.go` (or similar) typically contains **intentional contracts**, not elimination targets. The "corner cases" are guardrails for documented behavior. Before touching code that maps to such tests, verify you're not breaking the contract. (Exercise 7: metis `corner_cases_test.go` documents `aroundLatLng=""` → `[0,0]` etc. as public API.)

### 3. Precondition prologue

**Smell**: A block before a loop tests the same condition the loop will test.

**Distinction**: If the block **modifies state** that the loop depends on, it is precondition setup, not a guard. Between the prologue's test and the loop's test, the state has changed.

**Example** (`btree insert`): `if (is_full(root)) { create new root, split }` before the descent loop. This modifies `root` — without it, the loop invariant ("current node is not full") doesn't hold at entry.

**Distinguishable from #1**: a guard *tests without modifying*; a prologue *modifies state*.

### 4. Priority chain

**Smell**: An `if/else if/else` chain with 3+ branches looks like too many cases.

**Distinction**: Each branch handles a genuinely different precondition. The preconditions are mutually exclusive, and all branches lead to the same continuation code afterward.

**Example** (`btree insert/remove`): Try rotate-right → rotate-left → split/merge. Each handles a different sibling configuration. After any of them, the same descent code runs.

**Distinguishable from #7**: a priority chain has different preconditions leading to the same postcondition — the branches themselves are necessary. Transform-to-unify has one postcondition reached via an *initial transformation* then a single path.

### 5. Dual-site check

**Smell**: The same condition (e.g., key equality) is tested at two different points in the code.

**Distinction**: The value can be discovered at structurally different locations — both checks cover different discovery paths.

**Example** (`btree insert`): Duplicate key is checked at internal nodes (catches promoted medians) and at the leaf (catches pre-existing keys). Removing either allows duplicates through a specific path.

**Distinguishable from #7**: #7 unifies branches into one path. Dual-site keeps both checks because they guard structurally different code paths.

---

## Detection rules

Practical tools for applying the catalog. Each is tied to a principle.

### Substitution test (Principle A)

Substitute boundary values (empty collection, zero index, null pointer) into the general-case code. Trace through: does it produce the same result as the special-case code? If yes, the special case is redundant (pattern #1). If the general case doesn't handle it, consider whether a transformation can make it do so (pattern #7).

### Postcondition trace (Principle B)

After a function call, look at the documented postcondition (or the function's body if docs are missing). What specifically did the function change? If your next code is re-scanning the data structure to rediscover that, you're violating pattern #6. If your next code is handling a state the contract says is impossible, you're in pattern #9.

### Condition-test rule (Principle C)

Between two tests on the same condition, there should exist a code path that modifies that condition. If there is one, the tests are NOT redundant — they operate on different state (pattern #3 if sequential, #4 if parallel, #5 if structurally separated). If there is no modifying code path, the tests ARE redundant (pattern #1).

### TDD green-step question (Principle A during TDD)

At each green step, ask: "what's the general rule that covers this test AND the previous ones?" — NOT "what `if` do I add for this test?". This prevents corner cases from forming during development. Exercise 2 showed that TDD doesn't inherently produce corner cases if each green step generalizes instead of special-casing.

---

## Connection to coding guidelines

| Guideline (CLAUDE.md) | Principle | How it applies |
|---|---|---|
| Minimize cyclomatic complexity | A | Fewer special cases = fewer branches |
| Minimize exit points | A | One general path instead of multiple early returns |
| Early returns OK when they don't conflict | C (#3) | Prologues that modify state are valid early-exit patterns |
| Condition-test rule | C | Mechanical detector for false positives |
| Pre/post conditions and invariants | B | Use contracts directly, don't re-derive or work around |
