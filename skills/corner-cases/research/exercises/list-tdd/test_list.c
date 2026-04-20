#include "list.h"

#include <assert.h>
#include <stdio.h>

// Helper: count elements after sentinel
static size_t length(const struct cell *l) {
  size_t n = 0;
  for (const struct cell *it = l->next; it != NULL; it = it->next)
    n++;
  return n;
}

// Helper: get element at position (0-based, after sentinel)
static struct cell *at(struct cell *l, size_t pos) {
  struct cell *it = l->next;
  for (size_t i = 0; i < pos && it != NULL; i++)
    it = it->next;
  return it;
}

// Test 1: insert into empty list
static void test_insert_empty(void) {
  struct cell sentinel = {.next = NULL};
  struct cell a;
  insert_at(&sentinel, &a, 0);
  assert(sentinel.next == &a);
  assert(a.next == NULL);
  assert(length(&sentinel) == 1);
  printf("  PASS: test_insert_empty\n");
}

// Test 2: insert at front of non-empty list (pos == 0)
static void test_insert_front(void) {
  struct cell sentinel, a, b;
  sentinel.next = &a;
  a.next = NULL;
  insert_at(&sentinel, &b, 0);
  assert(sentinel.next == &b);
  assert(b.next == &a);
  assert(a.next == NULL);
  assert(length(&sentinel) == 2);
  printf("  PASS: test_insert_front\n");
}

// Test 3: insert in the middle (0 < pos < length)
static void test_insert_middle(void) {
  struct cell sentinel, a, b, c;
  sentinel.next = &a;
  a.next = &b;
  b.next = NULL;
  // list: a -> b, insert c at pos 1 (between a and b)
  insert_at(&sentinel, &c, 1);
  assert(at(&sentinel, 0) == &a);
  assert(at(&sentinel, 1) == &c);
  assert(at(&sentinel, 2) == &b);
  assert(length(&sentinel) == 3);
  printf("  PASS: test_insert_middle\n");
}

// Test 4: insert at the end (pos == length)
static void test_insert_end(void) {
  struct cell sentinel, a, b;
  sentinel.next = &a;
  a.next = NULL;
  // list: a, insert b at pos 1 (== length)
  insert_at(&sentinel, &b, 1);
  assert(at(&sentinel, 0) == &a);
  assert(at(&sentinel, 1) == &b);
  assert(b.next == NULL);
  assert(length(&sentinel) == 2);
  printf("  PASS: test_insert_end\n");
}

// Test 5: insert beyond the end (pos > length)
static void test_insert_beyond(void) {
  struct cell sentinel, a, b;
  sentinel.next = &a;
  a.next = NULL;
  // list: a, insert b at pos 100 (way beyond end)
  insert_at(&sentinel, &b, 100);
  assert(at(&sentinel, 0) == &a);
  assert(at(&sentinel, 1) == &b);
  assert(b.next == NULL);
  assert(length(&sentinel) == 2);
  printf("  PASS: test_insert_beyond\n");
}

// Test 6: multiple sequential inserts
static void test_insert_sequence(void) {
  struct cell sentinel = {.next = NULL};
  struct cell a, b, c, d;
  insert_at(&sentinel, &a, 0);   // [a]
  insert_at(&sentinel, &b, 0);   // [b, a]
  insert_at(&sentinel, &c, 1);   // [b, c, a]
  insert_at(&sentinel, &d, 100); // [b, c, a, d]
  assert(at(&sentinel, 0) == &b);
  assert(at(&sentinel, 1) == &c);
  assert(at(&sentinel, 2) == &a);
  assert(at(&sentinel, 3) == &d);
  assert(length(&sentinel) == 4);
  printf("  PASS: test_insert_sequence\n");
}

int main(void) {
  printf("Running insert_at tests:\n");
  test_insert_empty();
  test_insert_front();
  test_insert_middle();
  test_insert_end();
  test_insert_beyond();
  test_insert_sequence();
  printf("All tests passed.\n");
  return 0;
}
