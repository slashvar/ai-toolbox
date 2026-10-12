# Exercises

Worked examples that fed the `corner-cases` skill. Each is the smallest codebase that exhibits a specific flavor of the anti-pattern.

| Exercise                 | What it demonstrates                                                                                                                                                      |
|--------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`list/`](list/)         | C sentinel linked list — classic redundant boundary guards around a traversal loop. Refactoring surface: `insert_at`.                                                     |
| [`list-tdd/`](list-tdd/) | Same target as `list/`, built from scratch via TDD. Shows "generalize, don't guard" at each green step.                                                                   |
| [`btree/`](btree/)       | C++ top-down B-tree insertion with rotation, split, and duplicate handling — built TDD. Shows when `continue` after a structural mutation introduces a state-machine bug. |
| [`spanner/`](spanner/)   | C++14 header-only WSPD / spanner clustering library (Marwan's own, BSD-2-Clause 2017). Dummy-parameter overload selection, dead code, and default-parameter unification.  |

## File-naming convention

Used inside each exercise directory (from the project's `CLAUDE.md`):

- `*-orig.<ext>` — the starting file. Do not modify.
- `*-ref.<ext>` — a snapshot taken after the main refactor, before running `/simplify`. Used for before/after comparison.
- `*.<ext>` — the current working file.
