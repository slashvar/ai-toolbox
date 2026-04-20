#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

struct cell {
  struct cell *next;
};

void insert_at(struct cell *l, struct cell *e, size_t pos);

#endif
