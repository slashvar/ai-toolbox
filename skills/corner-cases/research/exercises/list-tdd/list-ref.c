#include "list.h"

void insert_at(struct cell *l, struct cell *e, size_t pos) {
  struct cell *it = l;
  for (size_t cur = 0; it->next != NULL && cur < pos; it = it->next) {
    cur++;
  }
  e->next = it->next;
  it->next = e;
}
