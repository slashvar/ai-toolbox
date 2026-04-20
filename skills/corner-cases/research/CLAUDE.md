# Learning to avoid corner case programming

Corner case programming is an anti-pattern of code where the code is split toward too many specific sub-cases that could have been handle directly with the main case.

**Goal:** we want to build a skill for coding agents that will help eliminate or avoid corner case programming.

Directory conventions:
* Code files are not the end goal, they are example to work with.
* Files with suffix `-orig.<extension>` are original files that should not be modified (`<extension>` should be replaced with specific language file extension)

Coding guidelines:
* We should try to minimize cyclomatic complexity
* We should try to minimize the number of exit points
* We should enforce the early return pattern as long as it does not go against the two previous points
* Between two tests on the same condition, it should exist a code path modifying that condition
* Reason through pre/post conditions and invariants: after applying a transformation, ensure invariants are respected using the transformation's known contract before moving forward (don't re-derive state from scratch when the contract tells you what changed)

Per-exercise workflow:
* Work through the exercise (refactoring or TDD red-green-refactor)
* Update `PATTERNS.md` and `LEARNINGS.md` with findings
* Run `/simplify` as a back-check: review the results for anything relevant to corner case elimination and record any insights in `LEARNINGS.md`
* Files with suffix `-ref.<extension>` are snapshots taken before the simplify pass for comparison