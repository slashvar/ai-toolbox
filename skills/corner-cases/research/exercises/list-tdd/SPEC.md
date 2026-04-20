# Spec: `insert_at` for sentinel-based intrusive linked list

## Data structure

```c
struct cell {
  struct cell *next;
};
```

A linked list is represented by a **sentinel** node. The sentinel's `next` pointer points to the first real element, or is `NULL` for an empty list. The last element's `next` is `NULL`.

## Function: `insert_at`

```c
void insert_at(struct cell *l, struct cell *e, size_t pos);
```

### Preconditions
- `l` is a valid sentinel (never NULL)
- `e` is a valid cell not already in the list
- `e->next` is undefined on entry (the function sets it)

### Behavior
Insert `e` into the list headed by sentinel `l` after `pos` positions from the sentinel.

- `pos == 0`: insert immediately after the sentinel (front of the list)
- `pos == k` where `k < length`: insert after the k-th element
- `pos >= length`: insert at the end of the list (append)

### Postconditions
- `e` is in the list
- All elements that were before position `pos` remain in their original order before `e`
- All elements that were at or after position `pos` remain in their original order after `e`
- No element is lost or duplicated

## Test cases to cover

1. Insert into empty list (any pos)
2. Insert at front of non-empty list (pos == 0)
3. Insert in the middle (0 < pos < length)
4. Insert at the end (pos == length)
5. Insert beyond the end (pos > length)
6. Multiple sequential inserts
