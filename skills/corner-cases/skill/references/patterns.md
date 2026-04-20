# Pattern Catalog

8 patterns organized under the 3 principles from SKILL.md. Each pattern has a smell (what to spot), a fix (what to do), an example (from real exercises), and a "distinguishable from" note (to avoid confusion with adjacent patterns).

---

## Principle A: Write the general case, not the specific one

### 1. Redundant boundary guard

**Smell**: An `if` block near a loop tests the same condition the loop tests. Its body duplicates the general code. Two common variants:
- **Before a loop**: guards the case where the loop executes zero iterations. The post-loop code already handles it.
- **After a loop**: re-tests a loop exit condition. The subsequent general code already covers it.

**Fix**: Delete the guard. Verify by substituting boundary values (empty collection, zero index, null pointer) through the general-case code and confirming identical behavior.

**Example** (linked list `insert_at` refactor):
- Before the loop: `if (l->next == NULL) { splice; return; }` — the splice after the loop already handles the zero-iteration case.
- After the loop: `if (it->next == NULL) { splice; return; }` — the general splice `e->next = it->next; it->next = e` already sets `e->next` to NULL when `it->next` is NULL.

The refactored function collapsed to 7 lines with one exit point. A bonus: a hidden `cur` increment bug became visible once the guards were removed.

**Distinguishable from #3 (precondition prologue)**: a prologue *modifies state* that the loop depends on. A boundary guard tests without modifying.

---

### 7. Transform to unify

**Smell**: Multiple branches each do different setup but fall through to the same general logic. The branches exist because the problem has variants that all lead to the same operation.

**Fix**: Transform the input so all variants collapse into one path. Apply the transformation first, then let the general machinery handle the rest.

**Example 1** (B-tree remove): CLRS has 3 sub-cases for "key in internal node" (predecessor child fat, successor child fat, both minimum). Instead of branching, always swap with predecessor (3 lines), then fall through to the standard descent. The descent's fattening logic handles making the child viable. The 3 sub-cases become 1 transformation + the general descent.

**Example 2** (Go parser package): 4 merge functions (`mergeFilters`, `mergeAnalyticsTags`, `mergeDevFeatureFlags`, `mergeArrayFilters`) had the same 4-step pipeline (empty check, Fields lookup, setter closure, call). Only types and accumulators varied. Unified with a generic helper `mergeTyped[T any](field, value, fromURL, accumulate func(T))`. 72 lines → 39 lines.

**Tool mapping**: In Go, C++, Rust, etc., **generics/templates are this pattern's best tool for boilerplate unification**.

**Distinguishable from #4 (priority chain)**: transform-to-unify has one outcome reached via all paths after a preparatory step. A priority chain has different *preconditions* but the same postcondition — the branches themselves are necessary.

---

### 8. Overload selection

**Smell**: Multiple constructor/function overloads that differ only in optional features. Dummy parameters (often `bool`) select between overloads. Bodies are near-identical.

**Fix**: Consolidate into a single entry point with default parameters. The caller explicitly names what it wants instead of relying on overload resolution with dummy values.

**Example** (spanner_clustering C++14): `wspd` had 4 constructors for 2 independent options (custom splitter, auto-decompose). `graph` had 3 more, `builder` 2 more — 9 total. Consolidated to 3 (one per type) with default parameters. Dummy `bool` params disappeared.

**Distinguishable from #4 (priority chain)**: overload selection is API-level duplication for feature combinations; a priority chain is algorithm-level branching with mutually exclusive preconditions.

---

## Principle B: Use what you know about state

### 6. Use postconditions, don't re-scan

**Smell**: After a transformation, the caller re-scans the data structure to rediscover the new state — even though the contract specifies exactly what changed.

**Fix**: Use the transformation's known effect directly (O(1) comparison) instead of re-scanning (O(log n) search).

**Example** (B-tree insert): After `split_child(cur, i)`, the median is at `cur->keys[i]`. Compare directly: `if (key == cur->keys[i]) return; if (key > cur->keys[i]) i++;` — instead of calling a general-purpose lookup.

**Corollary — no adjustment needed**: when a transformation doesn't invalidate your position (e.g., fattening a child you've already targeted), no adjustment is needed at all. Insert is SEARCHING (parent key changes can redirect); delete has already FOUND (position is fixed). Applies in B-tree remove: fattening rotations don't change the target child index — so unlike insert, no post-rotation key comparison is needed.

**Corollary — tracking flags**: a boolean stored to decide later control flow is often a symptom of missing postconditions. The `rotated` flag in one version of B-tree insert existed only to decide whether to re-scan afterward — using postconditions eliminated both the flag and the re-scan. **Not a smell**: flags that cache expensive computations (legitimate memoization) or flags that carry information across genuinely separate phases.

**Distinguishable from #9**: #6 is about using knowledge you HAVE (the contract). #9 is about handling knowledge you DON'T have (unclear contract).

---

### 9. Defensive code for unreachable states

**Smell**: Return statements or branches with "should not happen" comments handling cases the author believes are impossible.

**Fix**: Diagnose the root cause — one of two patterns applies:
- **Contract is well-established**: the defensive code is dead weight. Convert to an assertion (fail-fast) or remove.
- **Contract is unclear**: the defensive code works around a missing invariant. Fix the invariant upstream — then the state becomes provably unreachable.

**Example** (spanner_clustering): `parent()` returned `split_tree.root` with "should not happen." The root cause was upstream: `find_heads` could crash on leaf inputs, creating orphan nodes. Fixing `find_heads` made the defensive return provably unreachable.

**Distinguishable from #6**: #6 fixes by reading the contract; #9 fixes by repairing the contract.

---

## Principle C: When it looks redundant but isn't

These are **false positives** — patterns that look like corner case programming but are essential variation. Do NOT eliminate these. The condition-test rule from CLAUDE.md is the mechanical detector: "between two tests on the same condition, there should exist a code path modifying that condition."

**Heuristic for real-world code**: a test file named `corner_cases_test.*` typically contains **intentional contracts**, not elimination targets.

### 3. Precondition prologue

**Smell**: A block before a loop tests the same condition the loop will test.

**Distinction**: If the block **modifies state** that the loop depends on, it is precondition setup, not a guard. Between the prologue's test and the loop's test, the state has changed.

**Example** (B-tree insert): `if (is_full(root)) { create new root, split }` before the descent loop. This modifies `root` — without it, the loop invariant ("current node is not full") doesn't hold at entry.

**Distinguishable from #1**: a guard *tests without modifying*; a prologue *modifies state*.

---

### 4. Priority chain

**Smell**: An `if/else if/else` chain with 3+ branches looks like too many cases.

**Distinction**: Each branch handles a genuinely different precondition. The preconditions are mutually exclusive, and all branches lead to the same continuation code afterward.

**Example** (B-tree insert/remove): Try rotate-right → rotate-left → split/merge. Each handles a different sibling configuration. After any of them, the same descent code runs.

**Distinguishable from #7**: a priority chain has different preconditions leading to the same postcondition — the branches themselves are necessary. Transform-to-unify has one postcondition reached via an *initial transformation* then a single path.

---

### 5. Dual-site check

**Smell**: The same condition (e.g., key equality) is tested at two different points in the code.

**Distinction**: The value can be discovered at structurally different locations — both checks cover different discovery paths.

**Example** (B-tree insert): Duplicate key is checked at internal nodes (catches promoted medians) and at the leaf (catches pre-existing keys). Removing either allows duplicates through a specific path.

**Distinguishable from #7**: #7 unifies branches into one path. Dual-site keeps both checks because they guard structurally different code paths.
