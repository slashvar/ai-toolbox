---
name: corner-cases
description: Identifies and eliminates corner case programming — an anti-pattern where code is split into too many specific sub-cases that the general case already handles. Use this skill whenever the user asks to review, simplify, or refactor code; mentions "corner cases", "special cases", "edge cases", or "too many if statements"; has a function with many early returns or nested guards; wants to reduce cyclomatic complexity; or is doing TDD (red-green-refactor, test-first development) and needs to avoid introducing special-case branches at the green step. Also trigger when the user asks "am I overcomplicating this?", when code has repeated boundary guards before or after loops, when a codebase has many similar near-duplicate functions, or when defensive "should not happen" returns appear. Composes with TDD workflow skills (like `alg:tdd`) — corner-cases owns the code-shape rule at the green/refactor step, the workflow skill owns the cycle structure. Skip only for pure aesthetic formatting or when the code has been validated by the user as intentionally structured that way.
---

# Corner Case Elimination

Corner case programming is an anti-pattern where code is split into too many specific sub-cases that the general case already handles. This skill helps you identify true corner cases, eliminate them, and — just as importantly — recognize patterns that look redundant but are actually essential variation.

**Why this matters beyond aesthetics**: corner-case code masks bugs. Special-case branches bypass the general logic, hiding flaws in it. When you eliminate corner cases, latent bugs in the general path become visible and fixable.

## Two entry points

**Review/refactor mode** — user has existing code and wants it reviewed, simplified, or refactored. Apply the workflow below: scan, classify, fix true corner cases, document false positives.

**TDD green/refactor rule** — when user is writing code via TDD (whether guided by `alg:tdd` or another workflow), apply the code-shape rule at each green step and the refactor step. The TDD workflow skill (if active) owns cycle structure, test design, and when to refactor; this skill owns the rule for *what shape* the code should take. If no workflow skill is active, this rule still applies as a discipline inside whatever TDD loop the user is running.

## The 3 principles

### Principle A: Write the general case, not the specific one
The general logic, when written correctly, handles boundaries naturally. Corner cases arise when the programmer writes explicit handling for specific inputs instead of recognizing them as instances of the general case. **Patterns #1 (redundant boundary guard), #7 (transform to unify), #8 (overload selection)** live under this principle.

### Principle B: Use what you know about state
After any operation, you know more than you think. A function's contract tells you exactly what changed. An invariant established at entry holds throughout the loop. A "should not happen" state means the contract is unclear. Use this knowledge directly instead of re-deriving or working around it. **Patterns #6 (postconditions), #9 (defensive unreachable)** live here.

### Principle C: When it looks redundant but isn't
Two tests on the same condition are only redundant if they test the same state with the same outcome. Three patterns look redundant on the surface but are essential. **Patterns #3 (precondition prologue), #4 (priority chain), #5 (dual-site check)** — these are the **false positive** catalog. Do NOT eliminate these.

## Workflow (review/refactor mode)

1. **Scan for smells**. Read the target code function by function. For each function with repetition, early returns, nested guards, or branching, note it as a candidate.
2. **Classify each candidate** using the detection rules below. Is it a true corner case (Principle A/B) or essential variation (Principle C)?
3. **For true corner cases**: apply the pattern's fix. Verify by substituting boundary values through the general code. Run tests.
4. **For Principle C matches**: document WHY it looks suspicious but is essential. This becomes valuable review output — "I considered this; here's why it's the right shape."
5. **Iterate**: repeat per function, per file, per module.

## Rule for green/refactor steps (during any TDD workflow)

This skill does not run TDD for you — a workflow skill like `alg:tdd` owns cycle structure, test design, and when to refactor. This skill contributes one discipline to apply *inside* each cycle:

**At the green step** (making a failing test pass):
- **Ask: "What's the general rule that covers this test AND the previous ones?"** — NOT "what `if` do I add for this test?"
- If the new test can be passed by **generalizing** existing code (replacing it, expanding the loop condition, etc.), do that. When new code subsumes old code, delete the old code instead of guarding around it.
- If you catch yourself adding a boolean tracking flag to decide later control flow, stop. That flag is usually a symptom of not using the postconditions of a function you called. Read the function's contract; use what it tells you directly.

**At the refactor step** (after tests pass):
- Verify no new corner cases were introduced. Fold conditions into loops where possible; don't leave separate `if` blocks that the loop could absorb.
- Apply the detection rules below to any candidate repetition or branching that emerged during the green steps.

## Detection rules

Each rule is a concrete diagnostic for one principle.

### Substitution test (Principle A)
Substitute boundary values (empty collection, zero index, null pointer) into the general-case code. Trace through. Does it produce the same result as the special-case code? If yes, the special case is redundant — delete it (pattern #1). If the general case doesn't handle it cleanly, consider whether a small initial transformation (pattern #7) can make it do so.

### Postcondition trace (Principle B)
After a function call, look at the documented postcondition (or the function's body if docs are missing). What specifically did the function change? If your next code is re-scanning the data structure to rediscover what changed, you're violating pattern #6. If your next code is handling a state the contract says is impossible, you're in pattern #9 — fix the contract upstream.

### Condition-test rule (Principle C)
Between two tests on the same condition, there should exist a code path that modifies that condition. If there is one, the tests are NOT redundant — they operate on different state (pattern #3 if sequential, #4 if parallel, #5 if structurally separated). If there is no modifying code path between them, the tests ARE redundant (pattern #1).

### TDD green question (Principle A during TDD)
"What's the general rule that covers this test AND the previous ones?" — NOT "what `if` do I add for this test?". This prevents corner cases from forming during development.

## Pattern reference

For the full catalog of 8 patterns with smells, fixes, examples, and "distinguishable from" cross-references, read `references/patterns.md`. Consult it when:
- You identified a candidate via a detection rule and need the pattern's specific fix
- You're unsure which Principle C pattern applies to a false positive
- You want concrete examples of the pattern from real codebases (linked list, B-tree, spanner, metis)

## Heuristics for real-world code

- **A file named `corner_cases_test.*` (or similar) typically contains INTENTIONAL contracts, not elimination targets.** The "corner cases" in its name are guardrails for documented behavior. Before touching code that maps to such tests, verify you're not breaking the contract.
- **Repetition in exhaustive type dispatch is usually Principle C.** When a function has 100+ branches handling 100+ distinct types/fields (e.g., a `switch` over parameter names), each branch is genuinely different — that's not corner case programming, it's exhaustive handling. Don't try to DRY it up with reflection or codegen unless the benefit clearly exceeds the risk.
- **Tool mapping for Pattern #7**: in languages with generics (Go, C++, Rust), generics/templates are the best tool for unifying boilerplate-heavy near-duplicate functions. Use them when the pipeline is identical and only types/accumulators vary.
- **Scope discipline**: on large real-world code you don't own, prefer small, focused fixes over big structural refactors. The risk of breaking 40+ callers usually exceeds the clarity win.

## Output expectations

**Review/refactor mode**: produce (1) a list of findings classified as "corner case" or "essential variation", (2) for each corner case, the specific fix applied with before/after, (3) a note on what wasn't changed and why.

**During a TDD cycle**: at each green step, produce the smallest code change that passes the new test AND all previous tests, generalized rather than guarded. Flag any temptation to add a guard or flag and explain why the alternative is better. At the refactor step, apply the review detection rules to anything that looks suspicious.

## Connection to coding guidelines

| Guideline | Applies to | How |
|---|---|---|
| Minimize cyclomatic complexity | Principle A | Fewer special cases = fewer branches |
| Minimize exit points | Principle A | One general path instead of multiple early returns |
| Early returns OK when they don't conflict | Principle C (#3) | Prologues that modify state are valid |
| Condition-test rule | Principle C | Mechanical detector for false positives |
| Reason through pre/post conditions and invariants | Principle B | Use contracts directly |
