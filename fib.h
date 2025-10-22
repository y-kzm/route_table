#ifndef FIB_H
#define FIB_H

#define K 2
#define BRANCH_SZ (1 << K)

struct fib_node
{
  struct fib_node *child[BRANCH_SZ];
  int leaf; // 0: non-leaf, 1: leaf
  int plen;
  void *data;
};

struct fib_tree
{
  struct fib_node *root;
};

struct fib_tree *fib_new (struct fib_tree *t);
void fib_free (struct fib_tree *t);
int fib_route_add (struct fib_tree *t, const uint8_t *key, int plen, void *data);
int fib_route_delete (struct fib_tree *t, uint8_t *key, int plen);
struct fib_node * fib_route_lookup (struct fib_tree *t, const uint8_t *key);

#endif /* FIB_H */