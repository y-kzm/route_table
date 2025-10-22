#ifndef TEST_H
#define TEST_H

struct fib_tree *test_load_routes(const char *routes_filename);
int test_basic(struct fib_tree *t, const char *lookup_addrs_filename);
int test_basic_delete(struct fib_tree *t, const char *lookup_addrs_filename, const char *delete_routes_filename);
int test_performance(struct fib_tree *t);

#endif /* TEST_H */