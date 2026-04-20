# Examples: Before/After Snippets

Concrete illustrations from the 7 exercises that produced this catalog. Use these to calibrate what "good" looks like when applying the patterns.

---

## Pattern #1 — Redundant boundary guard (C linked list)

### Before (19 lines, 3 exit points, hidden bug)

```c
void insert_at(struct cell *l, struct cell *e, size_t pos) {
  if (l->next == NULL) {              // Guard 1 (before loop)
    l->next = e;
    e->next = NULL;
    return;
  }
  size_t cur = 0;
  struct cell *it = l;
  for (; it->next != NULL && cur < pos; it = it->next) {
    continue;                          // BUG: cur never incremented
  }
  if (it->next == NULL) {              // Guard 2 (after loop)
    it->next = e;
    e->next = NULL;
    return;
  }
  e->next = it->next;
  it->next = e;
}
```

### After (7 lines, 1 exit point)

```c
void insert_at(struct cell *l, struct cell *e, size_t pos) {
  struct cell *it = l;
  for (size_t cur = 0; it->next != NULL && cur < pos; it = it->next) {
    cur++;                             // Bug fix surfaced once guards removed
  }
  e->next = it->next;
  it->next = e;
}
```

**Key insight**: the general splice `e->next = it->next; it->next = e` handles all three cases (empty list, end of list, middle) uniformly. The two guards were redundant; removing them exposed a latent `cur` increment bug that had been masked.

---

## Pattern #7 — Transform to unify (B-tree remove, predecessor swap)

### Before (CLRS-style, 3 sub-cases for "key in internal node")

```
if key is in internal node at position j:
  if children[j] has > minimum keys:
    p = find_max(children[j])
    keys[j] = p
    recurse delete(p, children[j])
  else if children[j+1] has > minimum keys:
    s = find_min(children[j+1])
    keys[j] = s
    recurse delete(s, children[j+1])
  else:
    merge children[j] and children[j+1]
    recurse delete(key, merged_child)
```

### After (one transformation + unified descent)

```cpp
if (found && !is_leaf(cur)) {
    target = find_max(cur->children[idx]);
    cur->keys[idx] = target;
    i = idx;
    // fall through to the standard descent; fattening handles the child
}
```

**Key insight**: always swap with predecessor, then the standard descent loop (which pre-fattens children on the way down) handles what used to be 3 sub-cases. The fattening's priority chain (rotate-left / rotate-right / merge) is **not** a corner case — it's pattern #4 (genuine variation with the same continuation).

---

## Pattern #6 — Use postconditions (B-tree insert, post-split)

### Before (re-scan after every mutation, plus tracking flag)

```cpp
if (is_full(cur->children[i])) {
    bool rotated = false;
    if (i > 0 && !is_full(cur->children[i - 1])) {
        rotate_right(cur, i);
        rotated = true;
    } else if (i + 1 < cur->children.size() && !is_full(cur->children[i + 1])) {
        rotate_left(cur, i);
        rotated = true;
    }
    if (rotated) {
        std::tie(found, i) = find_or_index(cur, key);  // O(log K) re-scan
        if (found) return;
    }
    if (is_full(cur->children[i])) {
        split_child(cur, i);
        std::tie(found, i) = find_or_index(cur, key);  // another re-scan
        if (found) return;
    }
}
```

### After (use the known postcondition directly)

```cpp
if (is_full(cur->children[i])) {
    if (i > 0 && !is_full(cur->children[i - 1])) {
        rotate_right(cur, i);
        if (key == cur->keys[i - 1]) return;
        if (key < cur->keys[i - 1]) i--;
    } else if (i + 1 < cur->children.size() && !is_full(cur->children[i + 1])) {
        rotate_left(cur, i);
        if (key == cur->keys[i]) return;
        if (key > cur->keys[i]) i++;
    }
    if (is_full(cur->children[i])) {
        split_child(cur, i);
        if (key == cur->keys[i]) return;
        if (key > cur->keys[i]) i++;
    }
}
```

**Key insight**: each transformation (rotate_right, rotate_left, split_child) modifies exactly ONE parent key at a known position. Use a direct comparison (O(1)) instead of re-scanning (O(log K)). The `rotated` tracking flag disappears because it existed only to decide whether to re-scan.

---

## Pattern #8 — Overload selection (C++ spanner)

### Before (9 constructors across 3 types, dummy bool params)

```cpp
// wspd
wspd(PointSet<INFO>& set, double s)
  : Set(set), split_tree(Set), sep(s) { decompose(split_tree.root); }
wspd(PointSet<INFO>& set, double s, std::function<...>& splitter)
  : Set(set), split_tree(Set, splitter), sep(s) { decompose(split_tree.root); }
wspd(PointSet<INFO>& set, double s, bool)
  : Set(set), split_tree(Set), sep(s) {}
wspd(PointSet<INFO>& set, double s, std::function<...>& splitter, bool)
  : Set(set), split_tree(Set, splitter), sep(s) {}
// graph had 3 more, builder 2 more
```

### After (1 constructor with default parameters per type)

```cpp
wspd(PointSet<INFO>& set, double s,
     const std::function<void(tree<INFO>*)>& splitter = &tree<INFO>::seq_split,
     bool auto_decompose = true)
  : Set(set), split_tree(Set, splitter), sep(s) {
    if (auto_decompose) decompose(split_tree.root);
}
```

**Key insight**: overloads + dummy selectors express combinations of optional features. Default parameters say the same thing, explicitly, at the call site.

---

## Principle C in action — spotting FALSE positives on real code

### The metis queryparam 100-branch functions

In `queryparam/proto`, `Accept()`, `all()`, `byName()`, `Override()` each have ~100 `case` or `if` branches — one per parameter. An automated scanner would flag this as "extreme repetition." **It's not corner case programming.** Each parameter is a distinct field with a distinct type and serialization. Go's static type system doesn't offer a cheaper way to dispatch without reflection or codegen — which would cost more than they'd save on 40+ dependent callers.

**Classification**: Principle C (exhaustive variation). Keep as-is. Document the reasoning.

### The `corner_cases_test.go` heuristic

The metis `parse` package has a `corner_cases_test.go` file that documents specific behaviors: `aroundLatLng=""` → `[0,0]`, `insidePolygon=""` → `[]`, etc. A naive pattern-match would flag the empty-string guards in the decoders as "redundant boundary guards" (#1). **They're not.** They implement the documented API contract. Removing them breaks observable behavior.

**Rule**: when a test file is literally named "corner_cases_test", its contents are **contracts** — guardrails for the existing behavior, not targets for elimination.

---

## Pattern #7 with Go generics (queryparam/parse)

### Before (4 near-duplicate merge functions, ~72 lines)

Each followed the pattern: empty check, `Fields[key]` lookup, setter closure with type assertion, call `ToValueWithSetter`. Only the type and accumulator varied.

### After (1 generic helper + 4 thin callers, ~39 lines)

```go
func mergeTyped[T any](field, value string, fromURL bool, accumulate func(T)) error {
    if len(value) == 0 { return nil }
    parser, ok := Fields[field]
    if !ok { return unknownParameter(field) }
    setter := func(v any) error {
        accumulate(v.(T))
        return nil
    }
    return parser.ToValueWithSetter(value, fromURL, setter)
}

func (m *merger) mergeFilters(value string, fromURL bool) error {
    return mergeTyped("filters", value, fromURL, func(f string) {
        if len(m.filters) > 0 {
            m.filters = fmt.Sprintf("%s AND %s", m.filters, f)
        } else {
            m.filters = f
        }
    })
}
// similar for mergeAnalyticsTags, mergeDevFeatureFlags, mergeArrayFilters
```

**Key insight**: generics let each caller show only what's unique (field name + accumulation), with the pipeline extracted once. The variation becomes explicit and the boilerplate disappears.
