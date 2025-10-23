#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fib.h"

// key: address, s: start bit, n: number of bits
static inline uint32_t
BIT_INDEX32 (const uint8_t *key, int s, int n)
{
  uint32_t key32 = ((uint32_t)key[0] << 24) | ((uint32_t)key[1] << 16)
                   | ((uint32_t)key[2] << 8) | ((uint32_t)key[3]);
  return (((uint64_t)(key32) << 32 >> (64 - ((s) + (n)))) & ((1 << (n)) - 1));
}

struct fib_tree *
fib_new (struct fib_tree *t)
{
  if (!t)
    {
      t = malloc (sizeof (struct fib_tree));
      if (!t)
        return NULL;
    }
  t->root = NULL;
  return t;
}

static void
_free_fib_node (struct fib_node *n)
{
  int i;

  if (n != NULL)
    {
      for (i = 0; i < BRANCH_SZ; i++)
        _free_fib_node (n->child[i]);
      free (n);
    }
}

void
fib_free (struct fib_tree *t)
{
  if (t != NULL)
    {
      _free_fib_node (t->root);
      free (t);
    }
}

static inline struct fib_node *
_create_fib_node (void)
{
  struct fib_node *new;

  new = malloc (sizeof (struct fib_node));
  if (new != NULL)
    memset (new, 0, sizeof (struct fib_node));
  return new;
}

static struct fib_node *
_add (struct fib_node *n, const uint8_t *key, int plen, void *data, int depth)
{
  uint32_t index, i;
  uint32_t bits_in_depth, base, first, count;
  int exists = (n != NULL);

  if (!exists)
    n = _create_fib_node ();

  /* case1: 階層がプレフィックスに到達した場合 */
  if (plen <= depth)
    {
      /* 葉ノードではない（=子ノードが存在する）かつ、
       * 既にノードが存在する（=内部ノード）場合に
       * 全ての子に新しいプレフィックスを伝播 */
      if (!n->leaf && exists)
        {
          for (i = 0; i < BRANCH_SZ; i++)
            n->child[i] = _add (n->child[i], key, plen, data, depth + K);
          return n;
        }
      /* 葉ノードの場合 */
      else if (n->leaf)
        {
          /* より長いプレフィックスの場合に更新 */
          if (plen > n->plen)
            {
              n->plen = plen;
              n->data = data;
#ifdef DEBUG
              printf ("update leaf plen=%d, data=%p, depth=%d\n", plen, data,
                      depth);
#endif
            }
          return n;
        }
      /* 新規葉ノードとして登録 */
      else
        {
          n->leaf = 1;
          n->plen = plen;
          n->data = data;
#ifdef DEBUG
          printf ("set leaf plen=%d, data=%p, depth=%d\n", plen, data, depth);
#endif
          return n;
        }
    }

  /* case2: プレフィックスが次の階層の途中で終わる場合，もしくは葉ノードの場合 */
  if (plen < depth + K || n->leaf)
    {
      /*
       * - Example: K=2 (4-ary)
       *   - 96.0.0.0/3 (0b011/3)
       * ------------------------------------------
       *    v depth=0
       * root      v depth=2
       *    |---- 01      v depth=4
       *           |---- 00
       *           |---- 01
       *           |---- 10 <- new node: 96.0.0.0/3
       *           |---- 11 <- new node: 96.0.0.0/3
       * ------------------------------------------
       * - plen=3. depth=2 (3 < 2 + 2)
       *   - bits_in_depth: 3 - 2 = 1 (0b01|1*)
       *                                    ^
       *   - base = 0b01|10
       *               ^x1 = 1
       *   - first = 1 << (2 - 1) = 0b010 = 2
       *   - count = 1 << (2 - 1) = 0b010 = 2
       *   - range: child[2] to child[3]
       */
      /* 新しいプレフィックスが登録される子ノードの範囲を計算 */
      bits_in_depth = plen - depth; // この階層で決定されるビット数（1〜K-1）
      if (bits_in_depth > K - 1)
        bits_in_depth = K;
      base = BIT_INDEX32 (key, depth, bits_in_depth);
      first = base << (K - bits_in_depth); // 範囲の開始インデックス
      count = 1 << (K - bits_in_depth);    // 範囲のサイズ

      /* 全ての子ノードに対して */
      for (i = 0; i < BRANCH_SZ; i++)
        {
          if (n->leaf)
            /* 範囲外には親ノードのデータをコピー */
            n->child[i] = _add (n->child[i], key, n->plen, n->data, depth + K);
          if (i >= first && i < first + count)
            /* この範囲には新しいノードを登録 */
            n->child[i] = _add (n->child[i], key, plen, data, depth + K);
        }
      /* 現在のノードはもはや葉ノードではない */
      n->leaf = 0;
      n->plen = 0;
      n->data = NULL;
      return n;
    }

  /* case3: さらに深い階層へ再帰 */
  index = BIT_INDEX32 (key, depth, K);
#ifdef DEBUG
  printf ("depth=%d, index=%d\n", depth, index);
#endif
  n->child[index] = _add (n->child[index], key, plen, data, depth + K);
  return n;
}

int
fib_route_add (struct fib_tree *t, const uint8_t *key, int plen, void *data)
{
#ifdef DEBUG
  static int count = 0;
  count++;
  printf ("----- Add %d: key=%u.%u.%u.%u/%d, data=%p -----\n", count, key[0],
          key[1], key[2], key[3], plen, data);
#endif
  t->root = _add (t->root, key, plen, data, 0);
  return (t->root == NULL) ? -1 : 0; /* error(-1) if root is NULL */
}

#if 0 /* not implemented yet */
static int
_shrink (struct fib_node *n)
{
}

static struct fib_node *
_delete (struct fib_node *n, uint8_t *key, int plen, int depth)
{
}

int
fib_route_delete (struct fib_tree *t, uint8_t *key, int plen)
{
}
#endif /* 0 */

static struct fib_node *
_lookup (struct fib_node *n, struct fib_node *cand, const uint8_t *key,
         int depth)
{
  uint32_t index;

  if (n == NULL)
    return cand;

  if (n->leaf)
    cand = n;

  index = BIT_INDEX32 (key, depth, K);
#ifdef DEBUG
  printf ("depth=%d, index=%d\n", depth, index);
#endif

  return _lookup (n->child[index], cand, key, depth + K);
}

struct fib_node *
fib_route_lookup (struct fib_tree *t, const uint8_t *key)
{
  return _lookup (t->root, NULL, key, 0);
}