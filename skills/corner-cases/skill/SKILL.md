---
name: corner-cases
description: Identifies and eliminates corner case programming — an anti-pattern where code is split into too many specific sub-cases that the general case already handles. Use this skill whenever the user asks to review, simplify, or refactor code; mentions "corner cases", "special cases", "edge cases", or "too many if statements"; has a function with many early returns or nested guards; wants to reduce cyclomatic complexity; or is doing TDD (red-green-refactor, test-first development) and needs to avoid introducing special-case branches at the green step. Also trigger when the user asks "am I overcomplicating this?", when code has repeated boundary guards before or after loops, when a boolean expression lists sibling cases in different forms, when a codebase has many similar near-duplicate functions, or when defensive "should not happen" returns appear. Composes with TDD workflow skills — corner-cases owns the code-shape rule at the green/refactor step, the workflow skill owns the cycle structure. Skip only for pure aesthetic formatting or when the code has been validated by the user as intentionally structured that way.
---

# Corner Case Elimination

Corner case programming splits code into too many specific sub-cases that the general case already handles. This skill finds true corner cases, removes them, and recognizes patterns that look redundant but are essential variation.

**Why it matters beyond aesthetics**: special-case branches bypass the general logic and hide its flaws. Removing them exposes latent bugs in the general path.

## Entry points

| Mode                    | When                                    | What to do                                                                                         |
|-------------------------|-----------------------------------------|----------------------------------------------------------------------------------------------------|
| Review/refactor         | The user has code to review or simplify | Run the workflow below                                                                             |
| TDD green/refactor rule | The user is writing code test-first     | Apply the green/refactor rule inside whatever TDD loop is running; a workflow skill owns the cycle |

## The 3 principles

### A: Write the general case, not the specific one
The general logic, written correctly, handles boundaries naturally. Corner cases appear when the code names specific inputs instead of seeing them as instances of the general case.

**Patterns**: #1 redundant boundary guard, #7 transform to unify, #8 overload selection, #11 asymmetric sibling clauses.

### B: Use what you know about state
After an operation you know more than you think: a contract says what changed, an invariant holds through the loop, a "should not happen" state means the contract is unclear. Use that knowledge instead of re-deriving it.

**Patterns**: #6 postconditions, #9 defensive unreachable, #10 carried flag for a deferred decision, #12 implications between conditions.

### C: When it looks redundant but isn't
Two tests on one condition are redundant only if they test the same state with the same outcome. These look redundant but are essential: **#3 precondition prologue, #4 priority chain, #5 dual-site check**. This is the **false positive** catalog. Do NOT eliminate them.

## Workflow (review/refactor mode)

1. **Scan**: read function by function. Note each one with repetition, early returns, nested guards, or branching.
2. **Classify** each candidate with the detection rules: true corner case (A/B) or essential variation (C)?
3. **Fix** true corner cases with the pattern's fix. Substitute boundary values through the general code, then run tests.
4. **Gate behavior changes**: some fixes (#11, #12) change results for some inputs. List each changed input, write a failing test per change, and get user approval before applying.
5. **Document** Principle C matches: why each looks suspicious but is essential.
6. **Iterate** per function, file, module.

## Rule for green/refactor steps

At the **green step**:
- Ask: "What general rule covers this test AND the previous ones?", not "what `if` do I add for this test?".
- If generalizing existing code passes the test, do that. When new code subsumes old code, delete the old code.
- If you add a boolean to steer later control flow, stop. It usually means a function's postcondition is going unused. Read the contract and use it.

At the **refactor step**:
- Fold conditions into loops; do not leave `if` blocks the loop could absorb.
- Run the detection rules on any repetition or branching that emerged.

## Detection rules

### Substitution test (A)
Substitute boundary values (empty collection, zero index, null pointer) into the general code and trace. Same result as the special case? Delete the special case (#1). If the general code mishandles the value, try a small initial transformation (#7).

### Uniform-form test (A)
For each `||`/`&&` over sibling cases, write every term in the same form (state vs transition, presence vs presence-and-change) and compare results. Any input whose result differs is a hidden behavior difference (#11).

### Postcondition trace (B)
After a call, read its postcondition (or its body, if undocumented). Re-scanning to rediscover what changed is #6. Handling a state the contract calls impossible is #9: fix the contract upstream.

### Implication direction (B)
When `A` implies `G`, an early return on `!G` is safe and `G` can leave the conjuncts (#12). The reverse is not: never replace a term by something it implies, or by something that implies it. Run the reduced form against tests that pin the boundary inputs before accepting it.

### Condition-test rule (C)
Between two tests on one condition, some code path must modify that condition. If one exists, the tests are NOT redundant (#3 if sequential, #4 if parallel, #5 if structurally separate). If none exists, they ARE redundant (#1).

## Pattern reference

`references/patterns.md` holds the full catalog: smells, fixes, examples, and "distinguishable from" notes. Read it when:
- A detection rule found a candidate and you need the pattern's fix.
- You cannot tell which Principle C pattern a false positive is.
- You want a concrete example from real code (linked list, B-tree, spanner, Go services).

`references/examples.md` has before/after snippets.

## Heuristics for real-world code

- **A `corner_cases_test.*` file holds intentional contracts, not targets.** Before touching code those tests cover, check you are not breaking a documented behavior.
- **Exhaustive type dispatch is usually Principle C.** A `switch` with 100+ branches over distinct fields is exhaustive handling. Do not DRY it with reflection or codegen unless the gain clearly exceeds the risk.
- **Tools for #7**:
  - Generics/templates (Go, C++, Rust) unify near-duplicates whose pipeline is identical and only types or accumulators vary.
  - A nil-safe method (Go) makes "absent" the general case and removes a zero-value shadow variable.
- **Name variables by role, not by step.** Use `current`/`received`/`updated`, not `from`/`to`/`next`. Suffix the raw form (`currentJSON`). Drop variables that only shadow a field.
- **A guard kept for unmeasured performance is still a redundant guard (#1).**
- **Scope discipline**: on code you do not own, prefer small focused fixes over structural refactors. The risk to many callers usually exceeds the clarity gain.

## Output

**Review/refactor mode**: (1) findings classified "corner case" or "essential variation"; (2) for each corner case, the fix with before/after, plus the changed inputs and approval status for any behavior change; (3) what was left alone and why.

**TDD cycle**: at each green step, the smallest change that passes the new test and all previous ones, generalized rather than guarded. Name any guard or flag you were tempted to add and why the alternative is better. At the refactor step, run the detection rules.

## Connection to coding guidelines

| Guideline                                         | Principle | How                                               |
|---------------------------------------------------|-----------|---------------------------------------------------|
| Minimize cyclomatic complexity                    | A         | Fewer special cases, fewer branches               |
| Minimize exit points                              | A         | One general path instead of several early returns |
| Early returns OK when they don't conflict         | C (#3)    | Prologues that modify state are valid             |
| Condition-test rule                               | C         | Mechanical detector for false positives           |
| Reason through pre/post conditions and invariants | B         | Use contracts directly                            |
