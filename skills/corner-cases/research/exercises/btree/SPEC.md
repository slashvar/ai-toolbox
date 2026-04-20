# Spec: B-tree with Top-Down Insertion

## Data structure

```cpp
constexpr size_t K = 3;  // degree: max keys per node

struct node {
    std::vector<int> keys;
    std::vector<std::shared_ptr<node>> children;
};
```

A B-tree is represented by a `shared_ptr<node>` to the root. An empty tree is a single leaf node with empty `keys` and empty `children`. A node is a **leaf** when `children` is empty. A node is **full** when `keys.size() == K`.

## B-tree invariants

For degree K:

1. Every node has at most K keys
2. Every internal node (non-leaf, non-root) has at least ceil(K/2) keys
3. The root may have as few as 0 keys (empty tree) or 1 key (after first split)
4. A node with k keys and non-empty children has exactly k+1 children
5. Leaf nodes have 0 children; all leaves are at the same depth
6. Keys within each node are strictly sorted (ascending)
7. For index i: all keys in subtree `children[i]` are strictly less than `keys[i]`, and all keys in subtree `children[i+1]` are strictly greater than `keys[i]`
8. The tree grows by the root (splits propagate upward conceptually, but we use top-down pre-splitting)

## Functions

### Helpers

```cpp
bool is_full(const std::shared_ptr<node>& n);
bool is_leaf(const std::shared_ptr<node>& n);
size_t find_child_index(const std::shared_ptr<node>& n, int key);
```

- `is_full`: `keys.size() == K`
- `is_leaf`: `children.empty()`
- `find_child_index`: returns index of the child to descend into for `key` (using upper_bound on keys)

### `search`

```cpp
bool search(const std::shared_ptr<node>& root, int key);
```

Returns true if `key` exists anywhere in the tree.

### `split_child`

```cpp
void split_child(const std::shared_ptr<node>& parent, size_t index);
```

**Precondition**: `parent->children[index]` is full (K keys), `parent` is not full.

Splits `parent->children[index]` into two nodes:
- Left half keeps `keys[0..K/2-1]`
- Median `keys[K/2]` is promoted into `parent->keys` at position `index`
- New right node gets `keys[K/2+1..K-1]`
- Children (if internal) are split accordingly: left keeps `children[0..K/2]`, right gets `children[K/2+1..K]`
- The new right node is inserted into `parent->children` at position `index+1`

### `rotate_right`

```cpp
void rotate_right(const std::shared_ptr<node>& parent, size_t index);
```

**Precondition**: `index > 0`, `children[index]` has more than minimum keys, `children[index-1]` is not full.

Donates from `children[index]` to `children[index-1]` (left sibling):
- `children[index]->keys.front()` goes up to `parent->keys[index-1]`
- Old `parent->keys[index-1]` goes down to end of `children[index-1]->keys`
- If internal: `children[index]->children.front()` transfers to end of `children[index-1]->children`

### `rotate_left`

```cpp
void rotate_left(const std::shared_ptr<node>& parent, size_t index);
```

**Precondition**: `index < children.size()-1`, `children[index]` has more than minimum keys, `children[index+1]` is not full.

Mirror of rotate_right. Donates from `children[index]` to `children[index+1]` (right sibling):
- `children[index]->keys.back()` goes up to `parent->keys[index]`
- Old `parent->keys[index]` goes down to front of `children[index+1]->keys`
- If internal: `children[index]->children.back()` transfers to front of `children[index+1]->children`

### `insert`

```cpp
void insert(std::shared_ptr<node>& root, int key);
```

Top-down insertion. The `root` reference may be modified (new root creation when root is full).

**Behavior**:
- New keys are always added in a leaf node
- Maintains all B-tree invariants
- No duplicate keys: inserting an existing key is a no-op at the leaf, but may have restructured the tree on the way down
- After insertion, `search(root, key)` returns true

**Algorithm (top-down)**:
1. If root is full, create a new root, make old root its only child, split it
2. Descend: at each internal node, find the correct child
3. Before descending into a full child: try rotation to a non-full sibling, otherwise split
4. At the leaf, insert the key in sorted position (if not already present)

### `merge_children`

```cpp
void merge_children(std::shared_ptr<node>& parent, size_t index);
```

**Precondition**: `parent->children[index]` and `parent->children[index+1]` each have K/2 keys (minimum). `parent` has at least 1 key.

Inverse of `split_child`. Merges `children[index]` and `children[index+1]` with `parent->keys[index]` as separator into one node at position `index`. Parent loses one key and one child.

**Postcondition**: merged node at `parent->children[index]` has K keys. Parent has one fewer key and one fewer child.

### `remove`

```cpp
void remove(std::shared_ptr<node>& root, int key);
```

Top-down removal. The `root` reference may be modified (root shrinks when it becomes empty with one child).

**Behavior**:
- Removes `key` from the tree if present; no-op if absent
- Maintains all B-tree invariants
- After removal, `search(root, key)` returns false

**Algorithm (top-down with case unification)**:
1. Descend: at each internal node, find the key or the correct child
2. If key found at internal node: swap with in-order predecessor (max in left subtree), then continue descent to delete the predecessor
3. Before descending into a child at minimum: try rotation from a sibling, otherwise merge with a sibling
4. At the leaf: erase the key if present
5. Epilogue: if root has 0 keys and children, shrink (root = root->children[0])

### Helpers

```cpp
bool is_minimum(const std::shared_ptr<node>& n);  // keys.size() <= K/2
int find_max(const std::shared_ptr<node>& n);      // rightmost key in subtree
```

### `check_invariants`

```cpp
bool check_invariants(const std::shared_ptr<node>& root);
```

Test helper. Verifies all B-tree invariants hold. Returns true if valid.

## Test cases for `insert`

1. Insert into empty tree
2. Insert second key (sorted order maintained)
3. Insert third key (node at capacity)
4. Insert into full root (triggers root split)
5. Descent into non-full child
6. Insert duplicate key (no-op)
7. Child split during descent
8. Rotation instead of split
9. Duplicate equals promoted median
10. Ascending order insertion (1..9)
11. Descending order insertion (9..1)
12. Scrambled 20+ keys with invariant checks

## Test cases for `remove`

1. Remove from single leaf (middle, first, last key)
2. Remove key not present (no-op)
3. Remove only key from root (empty tree)
4. Remove leaf key via descent (no fattening)
5. Remove key from internal node (predecessor swap)
6. Fatten from left sibling (rotation)
7. Fatten from right sibling (rotation)
8. Fatten via merge + root shrink
9. Internal key with both children at minimum (swap + merge)
10. Three-level tree operations
11. Remove all keys one by one (forward and reverse)
12. Scrambled insert then remove with invariant checks
13. Interleaved insert/remove
14. Remove from empty tree (no-op)
