#ifndef BTREE_HPP
#define BTREE_HPP

#include <memory>
#include <vector>

constexpr size_t K = 3;

struct node {
    std::vector<int> keys;
    std::vector<std::shared_ptr<node>> children;
};

// Helpers
bool is_full(const std::shared_ptr<node>& n);
bool is_leaf(const std::shared_ptr<node>& n);
bool is_minimum(const std::shared_ptr<node>& n);
int find_max(const std::shared_ptr<node>& n);

// Operations
bool search(const std::shared_ptr<node>& root, int key);
void split_child(std::shared_ptr<node>& parent, size_t index);
void rotate_right(std::shared_ptr<node>& parent, size_t index);
void rotate_left(std::shared_ptr<node>& parent, size_t index);
void merge_children(std::shared_ptr<node>& parent, size_t index);
void insert(std::shared_ptr<node>& root, int key);
void remove(std::shared_ptr<node>& root, int key);

// Test helper
bool check_invariants(const std::shared_ptr<node>& root);

#endif
