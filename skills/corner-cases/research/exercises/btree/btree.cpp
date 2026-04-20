#include "btree.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

// Helpers

bool is_full(const std::shared_ptr<node>& n) {
    return n->keys.size() == K;
}

bool is_leaf(const std::shared_ptr<node>& n) {
    return n->children.empty();
}

// Returns {true, _} if key found, or {false, child_index} otherwise.
static std::pair<bool, size_t> find_or_index(const std::shared_ptr<node>& n, int key) {
    auto it = std::lower_bound(n->keys.begin(), n->keys.end(), key);
    if (it != n->keys.end() && *it == key)
        return {true, static_cast<size_t>(it - n->keys.begin())};
    return {false, static_cast<size_t>(it - n->keys.begin())};
}

// Search: descend the tree looking for key
bool search(const std::shared_ptr<node>& root, int key) {
    auto cur = root;
    while (cur) {
        auto [found, i] = find_or_index(cur, key);
        if (found) return true;
        if (is_leaf(cur)) return false;
        cur = cur->children[i];
    }
    return false;
}

// Split parent->children[index] (must be full) into two nodes,
// promoting the median key into parent.
void split_child(std::shared_ptr<node>& parent, size_t index) {
    auto child = parent->children[index];
    size_t mid = K / 2;

    // New right node gets keys after the median
    auto right = std::make_shared<node>();
    right->keys.assign(child->keys.begin() + mid + 1, child->keys.end());

    // Transfer children to right node if internal
    if (!is_leaf(child)) {
        right->children.assign(child->children.begin() + mid + 1,
                               child->children.end());
        child->children.erase(child->children.begin() + mid + 1,
                              child->children.end());
    }

    // Promote median into parent
    int median = child->keys[mid];
    parent->keys.insert(parent->keys.begin() + index, median);
    parent->children.insert(parent->children.begin() + index + 1, right);

    // Truncate left child to keys before the median
    child->keys.erase(child->keys.begin() + mid, child->keys.end());
}

// Rotate: donate from children[index] to children[index-1] (left sibling)
// through parent->keys[index-1]
void rotate_right(std::shared_ptr<node>& parent, size_t index) {
    auto src = parent->children[index];
    auto dst = parent->children[index - 1];

    // Parent key goes down to end of left sibling
    dst->keys.push_back(parent->keys[index - 1]);

    // Source's first key goes up to parent
    parent->keys[index - 1] = src->keys.front();
    src->keys.erase(src->keys.begin());

    // Transfer leftmost child if internal
    if (!is_leaf(src)) {
        dst->children.push_back(src->children.front());
        src->children.erase(src->children.begin());
    }
}

// Rotate: donate from children[index] to children[index+1] (right sibling)
// through parent->keys[index]
void rotate_left(std::shared_ptr<node>& parent, size_t index) {
    auto src = parent->children[index];
    auto dst = parent->children[index + 1];

    // Parent key goes down to front of right sibling
    dst->keys.insert(dst->keys.begin(), parent->keys[index]);

    // Source's last key goes up to parent
    parent->keys[index] = src->keys.back();
    src->keys.pop_back();

    // Transfer rightmost child if internal
    if (!is_leaf(src)) {
        dst->children.insert(dst->children.begin(), src->children.back());
        src->children.pop_back();
    }
}

bool is_minimum(const std::shared_ptr<node>& n) {
    return n->keys.size() <= K / 2;
}

int find_max(const std::shared_ptr<node>& n) {
    auto cur = n;
    while (!is_leaf(cur))
        cur = cur->children.back();
    return cur->keys.back();
}

// Inverse of split_child: merge children[index] and children[index+1]
// with parent->keys[index] as separator.
void merge_children(std::shared_ptr<node>& parent, size_t index) {
    auto left = parent->children[index];
    auto right = parent->children[index + 1];
    left->keys.push_back(parent->keys[index]);
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->children.insert(left->children.end(), right->children.begin(), right->children.end());
    parent->keys.erase(parent->keys.begin() + index);
    parent->children.erase(parent->children.begin() + index + 1);
}

void insert(std::shared_ptr<node>& root, int key) {
    // Prologue: ensure root is not full before descent
    if (is_full(root)) {
        auto new_root = std::make_shared<node>();
        new_root->children.push_back(root);
        split_child(new_root, 0);
        root = new_root;
    }
    // Descent to leaf
    auto cur = root;
    while (!is_leaf(cur)) {
        auto [found, i] = find_or_index(cur, key);
        if (found) return;
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
            // After rotation the target child may still be full (key shifted to receiving sibling)
            if (is_full(cur->children[i])) {
                split_child(cur, i);
                if (key == cur->keys[i]) return;
                if (key > cur->keys[i]) i++;
            }
        }
        cur = cur->children[i];
    }
    // Insert at leaf (skip if already present)
    auto it = std::lower_bound(cur->keys.begin(), cur->keys.end(), key);
    if (it == cur->keys.end() || *it != key) {
        cur->keys.insert(it, key);
    }
}

void remove(std::shared_ptr<node>& root, int key) {
    auto cur = root;
    int target = key;
    while (true) {
        auto [found, idx] = find_or_index(cur, target);
        if (is_leaf(cur)) {
            if (found) cur->keys.erase(cur->keys.begin() + idx);
            break;
        }
        size_t i = idx;
        if (found) {
            target = find_max(cur->children[idx]);
            cur->keys[idx] = target;
            i = idx;
        }
        // Fatten child if at minimum before descending
        if (is_minimum(cur->children[i])) {
            if (i > 0 && !is_minimum(cur->children[i - 1])) {
                rotate_left(cur, i - 1);
            } else if (i + 1 < cur->children.size() && !is_minimum(cur->children[i + 1])) {
                rotate_right(cur, i + 1);
            } else if (i > 0) {
                merge_children(cur, i - 1);
                i--;
            } else {
                merge_children(cur, i);
            }
        }
        cur = cur->children[i];
    }
    // Epilogue: shrink root if it became empty
    if (root->keys.empty() && !root->children.empty()) {
        root = root->children[0];
    }
}

// Invariant checker for tests
static bool check_node(const std::shared_ptr<node>& n,
                        int64_t lo, int64_t hi,
                        size_t expected_depth, size_t depth) {
    if (!n) return false;
    bool is_root = (depth == 0);

    // Key count bounds
    // Minimum applies to internal non-root nodes only; leaves and root can have fewer
    size_t min_keys = is_root ? 0 : K / 2;
    if (n->keys.size() > K) return false;
    if (n->keys.size() < min_keys) return false;

    // Keys must be strictly sorted and within (lo, hi)
    for (size_t i = 0; i < n->keys.size(); i++) {
        if (n->keys[i] <= lo || n->keys[i] >= hi) return false;
        if (i > 0 && n->keys[i] <= n->keys[i - 1]) return false;
    }

    if (is_leaf(n)) {
        return depth == expected_depth;
    }

    // Internal node: children count must be keys + 1
    if (n->children.size() != n->keys.size() + 1) return false;

    // Recurse into children with tightened bounds
    for (size_t i = 0; i < n->children.size(); i++) {
        int64_t child_lo = (i == 0) ? lo : static_cast<int64_t>(n->keys[i - 1]);
        int64_t child_hi = (i == n->keys.size()) ? hi : static_cast<int64_t>(n->keys[i]);
        if (!check_node(n->children[i], child_lo, child_hi,
                        expected_depth, depth + 1))
            return false;
    }
    return true;
}

// Compute leaf depth (all leaves must be at same depth)
static size_t leaf_depth(const std::shared_ptr<node>& n) {
    size_t d = 0;
    auto cur = n;
    while (!is_leaf(cur)) {
        cur = cur->children[0];
        d++;
    }
    return d;
}

bool check_invariants(const std::shared_ptr<node>& root) {
    if (!root) return false;
    constexpr auto lo = std::numeric_limits<int64_t>::min();
    constexpr auto hi = std::numeric_limits<int64_t>::max();
    return check_node(root, lo, hi, leaf_depth(root), 0);
}
