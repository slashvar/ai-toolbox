# Pattern Catalog

Patterns grouped under the 3 principles of `SKILL.md`. Each has a smell, a fix, an example, and a "distinguishable from" note. Numbers are stable IDs in discovery order, not a ranking; there is no #2.

---

## Principle A: Write the general case, not the specific one

### 1. Redundant boundary guard

**Smell**: An `if` near a loop tests a condition the loop already tests, and its body duplicates the general code.
- **Before the loop**: guards the zero-iteration case that the post-loop code already handles.
- **After the loop**: re-tests a loop exit condition that the following general code already covers.
- **Fast-path guard**: an early return the general path already covers, kept "for performance". A guard justified only by unmeasured cost is still redundant. Keep it only with a measurement.

**Fix**: Delete the guard. Substitute boundary values (empty collection, zero index, null pointer) through the general code and confirm identical behavior.

**Example** (linked list `insert_at`):
- Before the loop, `if (l->next == NULL) { splice; return; }`: the splice after the loop covers zero iterations.
- After the loop, `if (it->next == NULL) { splice; return; }`: the general splice `e->next = it->next; it->next = e` already sets `e->next` to NULL.

The function collapsed to 7 lines with one exit point. A hidden `cur` increment bug surfaced once the guards were gone.

**Distinguishable from #3**: a prologue *modifies state* the loop depends on; a boundary guard only tests.

---

### 7. Transform to unify

**Smell**: Several branches do different setup, then fall through to the same general logic.

**Fix**: Transform the input so all variants take one path. Apply the transformation first, then let the general machinery run.

**Example 1** (B-tree remove): CLRS has 3 sub-cases for "key in internal node" (predecessor child fat, successor child fat, both minimum). Always swap with the predecessor (3 lines) and fall through to the standard descent, whose fattening step makes the child viable.

**Example 2** (Go options parser): 4 merge functions shared one pipeline (empty check, schema lookup, setter closure, call). Only the type and accumulator varied. A generic helper `mergeTyped[T any](name, raw, accumulate func(T))` replaced them: 72 lines → 39.

**Tools**:
- **Generics/templates** (Go, C++, Rust) unify boilerplate-heavy near-duplicates.
- **A nil-safe method** (Go) makes "absent" the general case: with a nil receiver meaning "nothing provided", the `if x != nil` wrapper and the zero-value shadow variable disappear.

**Distinguishable from #4**: transform-to-unify reaches one outcome through a preparatory step. A priority chain has different *preconditions* with the same postcondition, and its branches are necessary.

---

### 8. Overload selection

**Smell**: Overloads that differ only in optional features, often selected by dummy (`bool`) parameters, with near-identical bodies.

**Fix**: One entry point with default parameters. The caller names what it wants instead of relying on overload resolution.

**Example** (C++ spanner library): `wspd` had 4 constructors for 2 independent options (custom splitter, auto-decompose); `graph` had 3 more and `builder` 2 more. Nine collapsed to three with default parameters, and the dummy `bool`s disappeared.

**Distinguishable from #4**: overload selection is API-level duplication over feature combinations. A priority chain is algorithm-level branching over mutually exclusive preconditions.

---

### 11. Asymmetric sibling clauses

**Smell**: A boolean expression lists sibling cases of one rule in different forms: one tests a *state*, the others a *transition*; or one tests *presence*, the others *presence and change*. The reader cannot tell whether the difference is intended. Usually it is not, and it hides a behavior difference nobody chose.

**Fix**:
1. Write every sibling in the same form.
2. List the inputs whose result changes.
3. Pin each with a test that fails before the rewrite, and get the behavior change approved.
4. Extract the uniform siblings into a named helper so the caller reads as one rule.

**Example** (settings write that must trigger a rebuild): "a write switches the feature on" was `!current.Enabled() || (!current.Beta && updated.Beta)`, a state test beside a transition test. Uniform, it is `(mode off→on) || (beta off→on)`, which also catches switching `mode` on when the feature is already on through `beta`. Likewise "a write reconfigures" tested `HasFields()` alone but `HasModel() && differs` and `HasParams() && differs`. Uniform "provided and changed" stops identical fields resent from triggering a rebuild. Both changes were pinned by new tests and approved before landing. See `examples.md`.

**Distinguishable from #1**: #1 deletes a branch the general case covers. #11 keeps every branch and makes them agree, which can change behavior, so it needs approval.

---

## Principle B: Use what you know about state

### 6. Use postconditions, don't re-scan

**Smell**: After a transformation, the caller re-scans the data to rediscover the new state, although the contract says exactly what changed.

**Fix**: Use the known effect directly (an O(1) comparison) instead of re-scanning (O(log n) search).

**Example** (B-tree insert): after `split_child(cur, i)` the median sits at `cur->keys[i]`. Compare directly (`if (key == cur->keys[i]) return; if (key > cur->keys[i]) i++;`) instead of calling a general lookup.

**Corollary, no adjustment needed**: when a transformation does not invalidate your position, adjust nothing. Insert is SEARCHING (a parent-key change can redirect it). Delete has already FOUND its target (the position is fixed), so fattening rotations in remove need no post-rotation comparison.

**Corollary, tracking flags**: a boolean stored to decide later control flow often means a missing postcondition. The `rotated` flag in one B-tree insert existed only to trigger a re-scan; using postconditions removed both. **Not a smell**: flags that memoize an expensive computation, or carry information across genuinely separate phases.

**Distinguishable from #9**: #6 uses knowledge you HAVE (the contract); #9 handles knowledge you DON'T (an unclear contract).

---

### 9. Defensive code for unreachable states

**Smell**: Returns or branches commented "should not happen", handling states the author believes impossible.

**Fix**: Find the root cause:
- **Contract is well-established**: the defensive code is dead weight. Convert it to an assertion or remove it.
- **Contract is unclear**: the code works around a missing invariant. Fix the invariant upstream, so the state becomes provably unreachable.

**Example** (spanner): `parent()` returned `split_tree.root` with "should not happen". The cause was upstream: `find_heads` could crash on leaf inputs and create orphan nodes. Fixing it made the defensive return provably unreachable.

**Distinguishable from #6**: #6 reads the contract; #9 repairs it.

---

### 10. Carried flag for a deferred decision

**Smell**: A decision is made where its inputs are in scope, but the code stores a *flag* (or a parallel field) and re-derives the result at a distant site. Tells:
- Two members track one fact (a `bool` plus the value it selects).
- A far-away branch reads the flag to rebuild what was already known.
- An `assert` at the use site re-proves an invariant the origin already guaranteed. Needing a runtime assert to re-establish a known fact is the signature of state spread too thin.

**Fix**: Resolve the value at its source and store the *result* (one field, not a flag). Make precedence explicit with an early return. If a later precondition gates the result, set it optimistically at the source and `reset()` it when the precondition fails.

**Example** (C++ request executor, degradation reporting): a request degrades from local worker saturation (report source `threads`) or from an incoming signal (source `upstream`), local winning. The first cut set `_maxCapacityReached` in the constructor plus a `bool _maxCapacityFromUpstream`, and a distant block rebuilt the report with `_degradation = _maxCapacityFromUpstream ? upstreamReport(*incoming) : {threads}`. That read as upstream precedence (misleading), needed `assert(!_maxCapacityFromUpstream || incoming != nullptr)` to re-prove the dereference, and split the decision from its inputs. The fix decides at the source: `_resolveDegradation()` sets `_degradation` directly (`if (localSaturated) { _degradation = {threads}; return; } if (signalActive) _degradation = upstreamReport(*incoming);`), and the block only gates: `_requestWasDegraded = applied; if (!applied) _degradation.reset();`. The flag, the ternary, and the assert vanish; precedence is structural.

**Distinguishable from #6**: #6 uses a postcondition to avoid re-scanning data; #10 stops a *decision* from travelling past its inputs. **From #9**: #9 repairs an invariant to remove defensive code; #10 moves the decision to where the invariant already holds.

---

### 12. Use implications between conditions

**Smell**: A guard `G` repeats as a conjunct on several branches, or the condition is `G && (A || B)` where `A` already implies `G`. The reader re-checks `G` everywhere.

**Fix**: When every case that matters implies `G`, return early on `!G` and drop `G` from the conjuncts. Then stop: an implication goes one way. Swapping a term for something it implies, or something that implies it, changes the result. Before any further reduction, run the substitution test on the boundary inputs and keep tests that pin them.

**Example** (settings write): `switchesOn(current, updated)` implies `updated.Enabled()`, so `if !updated.Enabled() { return nil }` is safe and the final condition shrinks to `switchesOn(...) || changes(...)`. The next proposed step, "after the early return `switchesOn` is just `!current.Enabled()`", was wrong: with `beta` already on, switching `mode` on still counts. Two existing tests failed on the reduced form, so it was not applied.

**Distinguishable from #6**: #6 uses what a call *guarantees afterwards*; #12 uses a logical relation *between conditions* in one expression.

---

## Principle C: When it looks redundant but isn't

These are **false positives**: essential variation that resembles corner case programming. Do NOT eliminate them. The condition-test rule is the mechanical detector: between two tests on the same condition, there must exist a code path that modifies it.

A test file named `corner_cases_test.*` usually holds **intentional contracts**, not elimination targets.

### 3. Precondition prologue

**Smell**: A block before a loop tests the condition the loop will test.

**Distinction**: if the block **modifies state** the loop depends on, it is setup, not a guard: the state changed between the two tests.

**Example** (B-tree insert): `if (is_full(root)) { create new root, split }` before the descent loop. Without it, the loop invariant ("current node is not full") fails at entry.

**Distinguishable from #1**: a guard tests without modifying; a prologue modifies.

---

### 4. Priority chain

**Smell**: An `if/else if/else` chain of 3+ branches looks like too many cases.

**Distinction**: each branch handles a genuinely different precondition, the preconditions are mutually exclusive, and all branches continue into the same code.

**Example** (B-tree insert/remove): try rotate-right, then rotate-left, then split/merge. Each handles a different sibling configuration, and the same descent runs after any of them.

**Distinguishable from #7**: a priority chain has different preconditions with one postcondition, so the branches are necessary. Transform-to-unify has an initial transformation, then a single path.

---

### 5. Dual-site check

**Smell**: The same condition (e.g., key equality) is tested at two points.

**Distinction**: the value can be discovered at structurally different locations, and each check covers a different discovery path.

**Example** (B-tree insert): a duplicate key is checked at internal nodes (catching promoted medians) and at the leaf (catching pre-existing keys). Removing either lets duplicates through one path.

**Distinguishable from #7**: #7 merges branches into one path; a dual-site check keeps both because they guard structurally different paths.
