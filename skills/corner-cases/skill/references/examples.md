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

---

# Additional examples (mined from real project history)

Later additions from real refactors across several projects, not the original 7 exercises. Same calibration purpose; each is verified against the source it came from.

---

## Principle C — essential vs redundant branches in one domain (float comparison)

From an IEEE-754 float-comparison library (C++, but the logic is portable to any IEEE language). Every comparison function carries special-case branches; the discipline is telling the **essential** ones from the **redundant** ones — both *look* like corner cases.

### Essential — keep (removing them breaks IEEE-754)

```cpp
// absolute & relative: matches ±Inf, because fabs(Inf - Inf) = NaN and NaN <= tol is false.
if (a == b) return true;

// relative only: scale = max(|a|,|b|) = Inf for any Inf input, and Inf*tol = Inf,
// so the general formula would spuriously match +Inf vs 1.0.
if (std::isinf(a) || std::isinf(b)) return false;

// ULPs: two NaN bit patterns can be adjacent as integers; arithmetic can't yield NaN here.
if (std::isnan(a) || std::isnan(b)) return false;

// ULPs: sign-magnitude integer distance is bogus across the sign boundary; the
// `return a == b` folds ±0 (signbits differ, values equal) into the same branch.
if (std::signbit(a) != std::signbit(b)) return a == b;
```

### Redundant — do NOT add (the general path already handles them)

- Explicit `isnan` in the absolute/relative compare — `NaN <= tol` is already false, so NaN falls through and fails the tolerance check.
- Explicit ±0 check anywhere — `+0 == -0` is already true.
- A separate "exact match" branch — that is just `a == b`.

**Key insight**: don't judge a guard by its syntax — check what the general path does with the values it screens. The `isinf`/`isnan`/`signbit` guards each defend against a value *class* the general formula provably mishandles (Inf arithmetic, NaN integer adjacency, cross-sign integer distance): essential, Principle C. The NaN/±0/exact-match guards defend against nothing IEEE arithmetic doesn't already reject, so adding them is corner-case programming — that's the #1 "general case already handles it" test. Same syntax, opposite verdicts.

---

## Pattern #1 — redundant vs essential boundary guard, same guard opposite verdict (graph diameter)

Two functions in a graph library each open with the *identical* guard `if (order == 0) ...`; neither modifies state, so both are #1 boundary guards (not #3 preconditions) — yet only one is removable.

```cpp
// full_diameter run(): the zero-vertex case already flows correctly through the
// general code (empty candidate loop → max_ecc stays 0 → returns diameter 0).
// This early return is a fast-path/clarity choice, not required for correctness.
const auto order = graph.order();
if (order == 0) {
    return { .diameter = 0, .radius = 0, .bfs_runs = 0, .connected = true };
}
```

```cpp
// cut-points: the body sizes vectors to `order` and unconditionally indexes
// vertex 0 (root = 0). With order == 0, prefix[0] / discovered[root] is
// out-of-bounds UB. Here the same-shaped guard is ESSENTIAL.
const auto order = graph.order();
if (order == 0) {
    return;
}
std::vector<char>        discovered(order, 0);   // zero-length
std::vector<std::size_t> prefix(order);          // prefix[0] would be UB
```

**Key insight**: #1's test for a boundary guard is *substituting the boundary value through the general path*, not matching the guard's shape. Do that here and the two diverge: `full_diameter`'s general path returns the right answer unaided (guard removable), while `cut-points`' general path indexes `prefix[0]` on an empty graph → UB. Same syntax, opposite verdict — the second is an essential boundary guard (Principle C), and it modifies nothing, so it is *not* a #3 precondition prologue.

---

## Pattern #9 — make the illegal state unrepresentable (Go type system)

A generic Go utility. This is #9 by its *fix*, not its classic smell: the runtime guard below is a live precondition check (not dead "should-not-happen" code), and the fix is to make the illegal input impossible to express.

### Before (runtime `reflect` guard)

```go
func UnmarshalOtherFields(data []byte, val any, otherFields *map[string]any) error {
    ptr := reflect.ValueOf(val)
    if ptr.Kind() != reflect.Ptr || ptr.Elem().Kind() != reflect.Struct {
        return errors.New("expected a pointer to struct")
    }
    ...
```

### After (`*T` makes the pointer a compile-time guarantee)

```go
func UnmarshalOtherFields[T any](data []byte, val *T, otherFields *map[string]any) error {
    ptr := reflect.ValueOf(val)
    if ptr.Elem().Kind() != reflect.Struct {   // the != reflect.Ptr check is gone
        return errors.New("expected a pointer to struct")
    }
    ...
```

**Key insight**: `val any` → `val *T` turns the runtime `!= reflect.Ptr` check into a compile-time guarantee, so that branch goes. (The `!= reflect.Struct` check stays — `*T` doesn't force `T` to be a struct.) It's #9's "repair the contract upstream," where the upstream is the *signature* itself — contrast the spanner `parent()` example, an upstream *algorithm* change.

---

## Pattern #7 — one generic erases a family of typed copies (Go)

### Before (per-type near-duplicates)

```go
func MinDuration(a, b time.Duration) time.Duration { if a < b { return a }; return b }
func MaxDuration(a, b time.Duration) time.Duration { if a < b { return b }; return a }
// plus MinInt / MinInt64 / ... elsewhere — the same body, once per type
```

### After (one generic over any ordered type; `Max` mirrors it)

```go
func Min[T cmp.Ordered](head T, tail ...T) T {
    res := head
    for _, x := range tail {
        if x < res { res = x }
    }
    return res
}
// Max is identical with `x > res`.
```

**Key insight**: each typed Min/Max is one algorithm specialized to a type — textbook #7, and the simplest form of it (contrast the callback-parameterized `mergeTyped` example above, where the *variation itself* also had to be abstracted). The sharper lesson is the coda: Go 1.21+ makes even this generic redundant with the builtin `min`/`max` — the general case sometimes migrates all the way into the language.

---

## Pattern #8 — optional callback replaces a duplicated overload (spanner)

Distinct from the wspd/graph *constructor* example (#8) earlier in this file — this is the `findpairs` recursion.

### Before (two near-identical recursive overloads; one fires an edge callback)

```cpp
void findpairs(box b1, box b2) {
    if (well_separated(b1, b2)) { addpair(b1, b2); return; }
    // ... recurse ...
}
void findpairs(box b1, box b2, std::function<void(box, box)>& edge) {
    if (well_separated(b1, b2)) { addpair(b1, b2); edge(b1, b2); return; }
    // ... recurse (duplicated body) ...
}
```

### After (one function, defaulted-null callable)

```cpp
void findpairs(box b1, box b2, std::function<void(box, box)> edge = nullptr) {
    if (well_separated(b1, b2)) {
        addpair(b1, b2);
        if (edge) edge(b1, b2);
        return;
    }
    // ... recurse (single copy) ...
}
```

**Key insight**: the overloads differed only by "also fire a callback." A default-null callable plus one `if (edge)` guard replaces the duplicated recursion body — the optional feature becomes an optional parameter, not a second function.

---

## Pattern #6 — cache the postcondition at the transform site (spanner)

### Before (every call re-scans the fixed `sizes` array, O(d))

```cpp
size_t maxd() {
    size_t m = 0;
    for (size_t i = 1; i < sizes.size(); i++)
        if (sizes[i] > sizes[m]) m = i;
    return m;
}
```

### After (compute once when the box geometry is finalized, then read)

```cpp
void update_max_dim() {
    max_dim = 0;
    for (size_t i = 1; i < sizes.size(); i++)
        if (sizes[i] > sizes[max_dim]) max_dim = i;
}
size_t maxd() { return max_dim; }
```

**Key insight**: the largest dimension is fixed once a box's `sizes` are set — a postcondition of the geometry update. Re-deriving it on every access is the re-scan smell (#6); compute it at the one transformation site and store it.

---

## Pattern #7 — lift the varying input into a producer (grammar)

Transform-to-unify applies to grammars as much as to functions. From a Bison parser.

### Before (two statement rules with near-identical actions, differing only in the RHS nonterminal)

```
tuple_assign_stmt
    : name_list '=' tuple_expr ';'  { /* build node */ }
    | name_list '=' expr       ';'  { /* SAME body, $3 is a plain expr */ }
```

### After (a `tuple_rhs` producer absorbs the variation; one rule, one body)

```
tuple_rhs
    : expr       { $$ = $1; }
    | tuple_expr { $$ = $1; }
    ;
tuple_assign_stmt
    : lvalue_list '=' tuple_rhs ';'  { /* one body */ }
```

**Key insight**: the two rules differed only in the *shape of the RHS input*. Lifting that difference into a `tuple_rhs` nonterminal collapses the duplicated action into one path. (The same change generalized `name_list` → `lvalue_list`, replacing identifier-only LHS with general expressions.)

---

## Pattern #6 — a tracking flag replaced by better-chosen state (ring buffer)

A fixed-size ring buffer reporting a ratio between its oldest and newest sample. The first cut tracked a write cursor plus a `filled bool` ("has the buffer wrapped yet?") — and reading the oldest element then needed a three-way branch.

### Before (write cursor + `filled` flag)

```go
type ratioWindow struct {
    samples []counterSample
    next    int  // next write position
    filled  bool // buffer has wrapped at least once
}

func (w *ratioWindow) push(errors, total int64) float64 {
    newest := counterSample{errors, total}
    w.samples[w.next] = newest
    w.next = (w.next + 1) % len(w.samples)
    if w.next == 0 {            // extra branch, only to maintain the flag
        w.filled = true
    }
    oldest, ok := w.oldest()
    if !ok {
        return 0
    }
    return ratio(oldest, newest)
}

func (w *ratioWindow) oldest() (counterSample, bool) {
    if w.filled {               // three-way branch to locate the oldest sample
        return w.samples[w.next], true
    }
    if w.next >= 2 {
        return w.samples[0], true
    }
    return counterSample{}, false
}
```

### After (oldest index + count; the flag and the whole `oldest()` helper vanish)

```go
type ratioWindow struct {
    samples []counterSample
    start   int // index of the oldest sample
    count   int // number of samples held (<= len(samples))
}

func (w *ratioWindow) push(errors, total int64) float64 {
    n := len(w.samples)
    newest := counterSample{errors, total}
    w.samples[(w.start+w.count)%n] = newest
    if w.count < n {
        w.count++
    } else {
        w.start = (w.start + 1) % n
    }
    if w.count < 2 {            // no interval spanned yet
        return 0
    }
    return ratio(w.samples[w.start], newest)
}
```

**Key insight**: `filled` was stored only to drive later control flow — the tracking-flag smell (#6). Choosing `count` over a bare cursor makes "full", "spans an interval", and the oldest index all *derivable*, so the flag, its maintenance `if`, and the whole three-way `oldest()` helper vanish. (Compare `maxd()` above: that #6 fix *caches* a postcondition; this one *picks state* so there's nothing to reconstruct.)
