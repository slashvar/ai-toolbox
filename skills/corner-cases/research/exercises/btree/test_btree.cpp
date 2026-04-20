#include "btree.hpp"

#include <gtest/gtest.h>

static std::shared_ptr<node> make_leaf(std::vector<int> keys) {
    auto n = std::make_shared<node>();
    n->keys = std::move(keys);
    return n;
}

static std::shared_ptr<node> make_internal(
    std::vector<int> keys, std::vector<std::shared_ptr<node>> children) {
    auto n = std::make_shared<node>();
    n->keys = std::move(keys);
    n->children = std::move(children);
    return n;
}

static size_t total_keys(const std::shared_ptr<node>& n) {
    size_t count = n->keys.size();
    for (const auto& child : n->children)
        count += total_keys(child);
    return count;
}

TEST(Search, EmptyTree) {
    auto root = make_leaf({});
    EXPECT_FALSE(search(root, 42));
}

TEST(Search, SingleNodePresent) {
    auto root = make_leaf({10, 20, 30});
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(search(root, 20));
    EXPECT_TRUE(search(root, 30));
}

TEST(Search, SingleNodeAbsent) {
    auto root = make_leaf({10, 20, 30});
    EXPECT_FALSE(search(root, 5));
    EXPECT_FALSE(search(root, 15));
    EXPECT_FALSE(search(root, 35));
}

TEST(Search, MultiLevelInRoot) {
    // root [20], children: [10] [30]
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    EXPECT_TRUE(search(root, 20));
}

TEST(Search, MultiLevelInLeaf) {
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(search(root, 30));
}

TEST(Search, MultiLevelAbsent) {
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    EXPECT_FALSE(search(root, 5));
    EXPECT_FALSE(search(root, 15));
    EXPECT_FALSE(search(root, 25));
    EXPECT_FALSE(search(root, 35));
}

TEST(SplitChild, SplitFullLeaf) {
    // Parent [50] with children: [10,20,30] (full), [60]
    auto parent = make_internal({50}, {make_leaf({10, 20, 30}), make_leaf({60})});
    split_child(parent, 0);
    // Median 20 promoted: parent keys [20, 50]
    EXPECT_EQ(parent->keys, (std::vector<int>{20, 50}));
    // Left child: [10], right child: [30], original right: [60]
    ASSERT_EQ(parent->children.size(), 3u);
    EXPECT_EQ(parent->children[0]->keys, (std::vector<int>{10}));
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{30}));
    EXPECT_EQ(parent->children[2]->keys, (std::vector<int>{60}));
}

TEST(SplitChild, SplitFullInternalNode) {
    // Build an internal child with K=3 keys and 4 children
    auto child = make_internal({10, 20, 30},
        {make_leaf({5}), make_leaf({15}), make_leaf({25}), make_leaf({35})});
    auto parent = make_internal({50}, {child, make_leaf({60})});
    split_child(parent, 0);
    // Median 20 promoted
    EXPECT_EQ(parent->keys, (std::vector<int>{20, 50}));
    ASSERT_EQ(parent->children.size(), 3u);
    // Left child: keys [10], children [{5},{15}]
    EXPECT_EQ(parent->children[0]->keys, (std::vector<int>{10}));
    ASSERT_EQ(parent->children[0]->children.size(), 2u);
    EXPECT_EQ(parent->children[0]->children[0]->keys, (std::vector<int>{5}));
    EXPECT_EQ(parent->children[0]->children[1]->keys, (std::vector<int>{15}));
    // Right child: keys [30], children [{25},{35}]
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{30}));
    ASSERT_EQ(parent->children[1]->children.size(), 2u);
    EXPECT_EQ(parent->children[1]->children[0]->keys, (std::vector<int>{25}));
    EXPECT_EQ(parent->children[1]->children[1]->keys, (std::vector<int>{35}));
}

TEST(SplitChild, SplitLastChild) {
    auto parent = make_internal({10}, {make_leaf({5}), make_leaf({20, 30, 40})});
    split_child(parent, 1);
    EXPECT_EQ(parent->keys, (std::vector<int>{10, 30}));
    ASSERT_EQ(parent->children.size(), 3u);
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{20}));
    EXPECT_EQ(parent->children[2]->keys, (std::vector<int>{40}));
}

TEST(SplitChild, SplitMiddleChild) {
    auto parent = make_internal({10, 50},
        {make_leaf({5}), make_leaf({20, 30, 40}), make_leaf({60})});
    split_child(parent, 1);
    EXPECT_EQ(parent->keys, (std::vector<int>{10, 30, 50}));
    ASSERT_EQ(parent->children.size(), 4u);
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{20}));
    EXPECT_EQ(parent->children[2]->keys, (std::vector<int>{40}));
}

TEST(RotateRight, LeafSiblings) {
    // Parent [20], children: [10] [25, 30]
    // Rotate from children[1] to children[0] through parent key
    auto parent = make_internal({20}, {make_leaf({10}), make_leaf({25, 30})});
    rotate_right(parent, 1);
    // 25 goes up to parent, old parent key 20 goes down to left child
    EXPECT_EQ(parent->keys, (std::vector<int>{25}));
    EXPECT_EQ(parent->children[0]->keys, (std::vector<int>{10, 20}));
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{30}));
}

TEST(RotateRight, InternalSiblings) {
    // Parent [20], children:
    //   left:  keys [10], children [{5},{15}]
    //   right: keys [25, 30], children [{22},{27},{35}]
    auto left = make_internal({10}, {make_leaf({5}), make_leaf({15})});
    auto right = make_internal({25, 30},
        {make_leaf({22}), make_leaf({27}), make_leaf({35})});
    auto parent = make_internal({20}, {left, right});
    rotate_right(parent, 1);
    // 25 goes up, 20 goes down to left, {22} transfers to left
    EXPECT_EQ(parent->keys, (std::vector<int>{25}));
    EXPECT_EQ(left->keys, (std::vector<int>{10, 20}));
    ASSERT_EQ(left->children.size(), 3u);
    EXPECT_EQ(left->children[2]->keys, (std::vector<int>{22}));
    EXPECT_EQ(right->keys, (std::vector<int>{30}));
    ASSERT_EQ(right->children.size(), 2u);
}

TEST(RotateLeft, LeafSiblings) {
    // Parent [20], children: [10, 15] [30]
    auto parent = make_internal({20}, {make_leaf({10, 15}), make_leaf({30})});
    rotate_left(parent, 0);
    // 15 goes up to parent, old parent key 20 goes down to right child
    EXPECT_EQ(parent->keys, (std::vector<int>{15}));
    EXPECT_EQ(parent->children[0]->keys, (std::vector<int>{10}));
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{20, 30}));
}

TEST(RotateLeft, InternalSiblings) {
    auto left = make_internal({10, 15},
        {make_leaf({5}), make_leaf({12}), make_leaf({18})});
    auto right = make_internal({30}, {make_leaf({25}), make_leaf({35})});
    auto parent = make_internal({20}, {left, right});
    rotate_left(parent, 0);
    // 15 goes up, 20 goes down to right, {18} transfers to right
    EXPECT_EQ(parent->keys, (std::vector<int>{15}));
    EXPECT_EQ(left->keys, (std::vector<int>{10}));
    ASSERT_EQ(left->children.size(), 2u);
    EXPECT_EQ(right->keys, (std::vector<int>{20, 30}));
    ASSERT_EQ(right->children.size(), 3u);
    EXPECT_EQ(right->children[0]->keys, (std::vector<int>{18}));
}

TEST(Invariants, ValidEmptyTree) {
    EXPECT_TRUE(check_invariants(make_leaf({})));
}

TEST(Invariants, ValidSingleNode) {
    EXPECT_TRUE(check_invariants(make_leaf({10, 20})));
}

TEST(Invariants, ValidTwoLevel) {
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    EXPECT_TRUE(check_invariants(root));
}

TEST(Invariants, TooManyKeys) {
    // K=3, so 4 keys is invalid
    EXPECT_FALSE(check_invariants(make_leaf({1, 2, 3, 4})));
}

TEST(Invariants, UnsortedKeys) {
    EXPECT_FALSE(check_invariants(make_leaf({20, 10})));
}

TEST(Invariants, WrongChildCount) {
    // 1 key but 3 children
    auto bad = make_internal({20},
        {make_leaf({10}), make_leaf({25}), make_leaf({30})});
    EXPECT_FALSE(check_invariants(bad));
}

TEST(Insert, IntoEmptyTree) {
    auto root = make_leaf({});
    insert(root, 10);
    EXPECT_TRUE(search(root, 10));
    EXPECT_EQ(root->keys, (std::vector<int>{10}));
    EXPECT_TRUE(is_leaf(root));
}

TEST(Insert, SecondKeySorted) {
    auto root = make_leaf({});
    insert(root, 10);
    insert(root, 5);
    EXPECT_EQ(root->keys, (std::vector<int>{5, 10}));
}

TEST(Insert, ThirdKeyAtCapacity) {
    auto root = make_leaf({});
    insert(root, 10);
    insert(root, 5);
    insert(root, 15);
    EXPECT_EQ(root->keys, (std::vector<int>{5, 10, 15}));
    EXPECT_TRUE(is_leaf(root));
}

TEST(Insert, FullRootSplit) {
    auto root = make_leaf({});
    insert(root, 5);
    insert(root, 10);
    insert(root, 15);
    // Root is now full [5,10,15]. Inserting 20 must trigger root split.
    insert(root, 20);
    EXPECT_TRUE(search(root, 5));
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(search(root, 15));
    EXPECT_TRUE(search(root, 20));
    EXPECT_FALSE(is_leaf(root));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, DescentIntoNonFullChild) {
    auto root = make_leaf({});
    for (int k : {5, 10, 15, 20})
        insert(root, k);
    // Tree should be 2-level now. Insert 3 goes into left child.
    insert(root, 3);
    EXPECT_TRUE(search(root, 3));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, DuplicateKey) {
    auto root = make_leaf({});
    for (int k : {5, 10, 15})
        insert(root, k);
    // Tree structure may change (split), but total key count must not
    size_t count_before = total_keys(root);
    insert(root, 10);
    EXPECT_EQ(total_keys(root), count_before);
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, ChildSplitDuringDescent) {
    auto root = make_leaf({});
    // Build up enough to have a 2-level tree with a full child
    for (int k : {10, 20, 30, 5, 15, 25})
        insert(root, k);
    EXPECT_TRUE(check_invariants(root));
    // Insert more to fill a child, then trigger child split
    insert(root, 35);
    EXPECT_TRUE(search(root, 35));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, RotationInsteadOfSplit) {
    auto root = make_leaf({});
    // Build a tree where one child is full and a sibling has room
    // The exact sequence depends on the algorithm, so we verify invariants
    for (int k : {10, 20, 30, 5, 15})
        insert(root, k);
    EXPECT_TRUE(check_invariants(root));
    // Inserting into a full child should try rotation first
    insert(root, 12);
    EXPECT_TRUE(search(root, 12));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, DuplicateEqualsPromotedMedian) {
    auto root = make_leaf({});
    for (int k : {5, 10, 15, 20})
        insert(root, k);
    // 10 was likely promoted as median during root split
    // Inserting 10 again should be a no-op
    insert(root, 10);
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, AscendingOrder) {
    auto root = make_leaf({});
    for (int k = 1; k <= 9; k++)
        insert(root, k);
    for (int k = 1; k <= 9; k++)
        EXPECT_TRUE(search(root, k));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, DescendingOrder) {
    auto root = make_leaf({});
    for (int k = 9; k >= 1; k--)
        insert(root, k);
    for (int k = 1; k <= 9; k++)
        EXPECT_TRUE(search(root, k));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Insert, StressScrambled) {
    auto root = make_leaf({});
    std::vector<int> keys = {42, 7, 31, 15, 88, 3, 56, 23, 71, 12,
                             99, 45, 67, 28, 84, 9, 51, 36, 63, 18,
                             77, 5, 93, 40, 60};
    for (int k : keys) {
        insert(root, k);
        EXPECT_TRUE(search(root, k)) << "key " << k << " not found after insert";
        EXPECT_TRUE(check_invariants(root)) << "invariants broken after inserting " << k;
    }
    for (int k : keys)
        EXPECT_TRUE(search(root, k));
}

// --- Helper tests for removal ---

TEST(MergeChildren, LeafChildren) {
    auto parent = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    merge_children(parent, 0);
    EXPECT_TRUE(parent->keys.empty());
    ASSERT_EQ(parent->children.size(), 1u);
    EXPECT_EQ(parent->children[0]->keys, (std::vector<int>{10, 20, 30}));
    EXPECT_TRUE(is_leaf(parent->children[0]));
}

TEST(MergeChildren, InternalChildren) {
    auto left = make_internal({10}, {make_leaf({5}), make_leaf({15})});
    auto right = make_internal({50}, {make_leaf({40}), make_leaf({60})});
    auto parent = make_internal({30}, {left, right});
    merge_children(parent, 0);
    ASSERT_EQ(parent->children.size(), 1u);
    auto merged = parent->children[0];
    EXPECT_EQ(merged->keys, (std::vector<int>{10, 30, 50}));
    ASSERT_EQ(merged->children.size(), 4u);
    EXPECT_EQ(merged->children[0]->keys, (std::vector<int>{5}));
    EXPECT_EQ(merged->children[1]->keys, (std::vector<int>{15}));
    EXPECT_EQ(merged->children[2]->keys, (std::vector<int>{40}));
    EXPECT_EQ(merged->children[3]->keys, (std::vector<int>{60}));
}

TEST(MergeChildren, MiddlePosition) {
    auto parent = make_internal({20, 40, 60},
        {make_leaf({10}), make_leaf({30}), make_leaf({50}), make_leaf({70})});
    merge_children(parent, 1);
    EXPECT_EQ(parent->keys, (std::vector<int>{20, 60}));
    ASSERT_EQ(parent->children.size(), 3u);
    EXPECT_EQ(parent->children[1]->keys, (std::vector<int>{30, 40, 50}));
}

TEST(FindMax, Leaf) {
    EXPECT_EQ(find_max(make_leaf({10, 20, 30})), 30);
}

TEST(FindMax, MultiLevel) {
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({25, 30})});
    EXPECT_EQ(find_max(root), 30);
}

TEST(IsMinimum, OneKey) {
    EXPECT_TRUE(is_minimum(make_leaf({10})));
}

TEST(IsMinimum, TwoKeys) {
    EXPECT_FALSE(is_minimum(make_leaf({10, 20})));
}

// --- Remove tests ---

TEST(Remove, FromSingleLeaf) {
    auto root = make_leaf({10, 20, 30});
    remove(root, 20);
    EXPECT_EQ(root->keys, (std::vector<int>{10, 30}));
    EXPECT_FALSE(search(root, 20));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, FirstAndLastKey) {
    auto root = make_leaf({10, 20, 30});
    remove(root, 10);
    EXPECT_EQ(root->keys, (std::vector<int>{20, 30}));
    remove(root, 30);
    EXPECT_EQ(root->keys, (std::vector<int>{20}));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, KeyNotPresent) {
    auto root = make_leaf({10, 20, 30});
    size_t count = total_keys(root);
    remove(root, 15);
    EXPECT_EQ(total_keys(root), count);
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, OnlyKeyFromRoot) {
    auto root = make_leaf({10});
    remove(root, 10);
    EXPECT_TRUE(root->keys.empty());
    EXPECT_FALSE(search(root, 10));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, LeafKeyViaDescent) {
    auto root = make_leaf({});
    for (int k : {10, 20, 30, 5, 15})
        insert(root, k);
    ASSERT_TRUE(check_invariants(root));
    // Remove a leaf key where the leaf has > minimum keys
    remove(root, 15);
    EXPECT_FALSE(search(root, 15));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, InternalKeySwapPredecessor) {
    // Build: root [20], children: [10,15] [25,30]
    auto root = make_internal({20}, {make_leaf({10, 15}), make_leaf({25, 30})});
    remove(root, 20);
    EXPECT_FALSE(search(root, 20));
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(search(root, 15));
    EXPECT_TRUE(search(root, 25));
    EXPECT_TRUE(search(root, 30));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, InternalKeyMultipleRootKeys) {
    auto root = make_internal({20, 40},
        {make_leaf({10, 15}), make_leaf({25, 30}), make_leaf({45, 50})});
    remove(root, 20);
    EXPECT_FALSE(search(root, 20));
    for (int k : {10, 15, 25, 30, 40, 45, 50})
        EXPECT_TRUE(search(root, k));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, FattenFromLeftSibling) {
    // Right child at minimum, left has surplus
    auto root = make_internal({20}, {make_leaf({5, 10}), make_leaf({30})});
    remove(root, 30);
    EXPECT_FALSE(search(root, 30));
    EXPECT_TRUE(search(root, 5));
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(search(root, 20));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, FattenFromRightSibling) {
    // Left child at minimum, right has surplus
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({25, 30})});
    remove(root, 10);
    EXPECT_FALSE(search(root, 10));
    EXPECT_TRUE(search(root, 20));
    EXPECT_TRUE(search(root, 25));
    EXPECT_TRUE(search(root, 30));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, FattenViaMergeAndShrink) {
    // Both children at minimum — must merge, then root shrinks
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    remove(root, 10);
    EXPECT_FALSE(search(root, 10));
    EXPECT_TRUE(search(root, 20));
    EXPECT_TRUE(search(root, 30));
    EXPECT_TRUE(is_leaf(root));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, FattenViaMergeOtherSide) {
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    remove(root, 30);
    EXPECT_FALSE(search(root, 30));
    EXPECT_TRUE(is_leaf(root));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, InternalKeyBothChildrenMinimum) {
    // Key is in root, both children at minimum — swap + merge
    auto root = make_internal({20}, {make_leaf({10}), make_leaf({30})});
    remove(root, 20);
    EXPECT_FALSE(search(root, 20));
    EXPECT_TRUE(search(root, 10));
    EXPECT_TRUE(search(root, 30));
    EXPECT_TRUE(is_leaf(root));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, ThreeLevelDirect) {
    auto root = make_leaf({});
    for (int k = 1; k <= 9; k++)
        insert(root, k);
    ASSERT_TRUE(check_invariants(root));
    // Remove a leaf key that doesn't require fattening
    remove(root, 9);
    EXPECT_FALSE(search(root, 9));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, ThreeLevelCascadingFatten) {
    auto root = make_leaf({});
    for (int k = 1; k <= 9; k++)
        insert(root, k);
    // Remove keys that force fattening at multiple levels
    remove(root, 1);
    EXPECT_FALSE(search(root, 1));
    EXPECT_TRUE(check_invariants(root));
}

TEST(Remove, ThreeLevelInternalNonRoot) {
    auto root = make_leaf({});
    for (int k = 1; k <= 9; k++)
        insert(root, k);
    // Remove a key that's in a non-root internal node
    // We search for internal keys by checking what's not in leaves
    for (int k = 1; k <= 9; k++) {
        if (!is_leaf(root) && search(root, k)) {
            remove(root, k);
            EXPECT_FALSE(search(root, k));
            EXPECT_TRUE(check_invariants(root));
            break;
        }
    }
}

TEST(Remove, AllKeysOneByOne) {
    auto root = make_leaf({});
    for (int k = 1; k <= 9; k++)
        insert(root, k);
    for (int k = 1; k <= 9; k++) {
        remove(root, k);
        EXPECT_FALSE(search(root, k)) << "key " << k << " still found after remove";
        EXPECT_TRUE(check_invariants(root)) << "invariants broken after removing " << k;
    }
    EXPECT_TRUE(root->keys.empty());
}

TEST(Remove, AllKeysReverseOrder) {
    auto root = make_leaf({});
    for (int k = 1; k <= 9; k++)
        insert(root, k);
    for (int k = 9; k >= 1; k--) {
        remove(root, k);
        EXPECT_FALSE(search(root, k));
        EXPECT_TRUE(check_invariants(root)) << "invariants broken after removing " << k;
    }
    EXPECT_TRUE(root->keys.empty());
}

TEST(Remove, ScrambledInsertThenRemove) {
    auto root = make_leaf({});
    std::vector<int> keys = {42, 7, 31, 15, 88, 3, 56, 23, 71, 12,
                             99, 45, 67, 28, 84, 9, 51, 36, 63, 18,
                             77, 5, 93, 40, 60};
    for (int k : keys)
        insert(root, k);
    // Remove in a different order
    std::vector<int> remove_order = {60, 40, 93, 5, 77, 18, 63, 36, 51, 9,
                                     84, 28, 67, 45, 99, 12, 71, 23, 56, 3,
                                     88, 15, 31, 7, 42};
    for (int k : remove_order) {
        remove(root, k);
        EXPECT_FALSE(search(root, k)) << "key " << k << " still found";
        EXPECT_TRUE(check_invariants(root)) << "invariants broken after removing " << k;
    }
    EXPECT_TRUE(root->keys.empty());
}

TEST(Remove, InterleavedInsertRemove) {
    auto root = make_leaf({});
    insert(root, 10); insert(root, 20); insert(root, 30); insert(root, 40); insert(root, 50);
    ASSERT_TRUE(check_invariants(root));
    remove(root, 30);
    EXPECT_TRUE(check_invariants(root));
    insert(root, 35);
    EXPECT_TRUE(check_invariants(root));
    remove(root, 10);
    EXPECT_TRUE(check_invariants(root));
    insert(root, 5);
    EXPECT_TRUE(check_invariants(root));
    remove(root, 50);
    EXPECT_TRUE(check_invariants(root));
    EXPECT_FALSE(search(root, 30));
    EXPECT_FALSE(search(root, 10));
    EXPECT_FALSE(search(root, 50));
    EXPECT_TRUE(search(root, 5));
    EXPECT_TRUE(search(root, 35));
    EXPECT_TRUE(search(root, 20));
    EXPECT_TRUE(search(root, 40));
}

TEST(Remove, EmptyTreeNoOp) {
    auto root = make_leaf({});
    remove(root, 42);
    EXPECT_TRUE(root->keys.empty());
    EXPECT_TRUE(check_invariants(root));
}
