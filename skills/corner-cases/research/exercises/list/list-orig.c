/*
 * Naive list implementation
 */

#include <stdlib.h>

// cell represents a cell of an intrusive linked list.
struct cell {
  struct cell *next;
};

// length computes the length of the list (skipping sentinel)
size_t length(const struct cell *l) {
  size_t sz = 0;
  for (struct cell *it = l; it->next != NULL; it = it->next) {
    ++sz;
  }
  return sz;
}

// push_front adds a cell in front (after the sentinel)
void push_front(struct cell *l, struct cell *e) {
  e->next = l->next;
  l->next = e;
}

// insert_at inserts element in l after pos position, or at the end if the list
// is smaller.
void insert_at(struct cell *l, struct cell *e, size_t pos) {
  if (l->next == NULL) {
    l->next = e;
    e->next = NULL;
    return;
  }
  size_t cur = 0;
  struct cell *it = l;
  for (; it->next != NULL && cur < pos; it = it->next) {
    continue;
  }
  if (it->next == NULL) {
    it->next = e;
    e->next = NULL;
    return;
  }
  e->next = it->next;
  it->next = e;
}