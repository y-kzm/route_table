#include <arpa/inet.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "fib.h"

#define LINE_BUF_SIZE 4096
#define IP_BUF_SIZE 64

/* -------------------------------------------
 * Utilities
 * ------------------------------------------- */

/* Xorshift32 RNG */
/* see: https://github.com/drpnd/radix-tree/blob/master/tests/basic.c */
static inline uint32_t
xorshift32 (void)
{
  static uint32_t s = 0x9E3779B9u;
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return s;
}

/* Elapsed time in seconds (double) */
/* see: https://www.cc.u-tokyo.ac.jp/public/VOL8/No5/data_no2_0609.pdf */
static double
now_seconds (void)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

static inline void
uint32_to_ipv4_bytes_hton (uint32_t host_ip, uint8_t out[4])
{
  uint32_t net_ip = htonl (host_ip);
  memcpy (out, &net_ip, 4);
}

static inline uint32_t
ipv4_bytes_to_uint32_ntoh (const uint8_t addr[4])
{
  uint32_t net_ip;
  memcpy (&net_ip, addr, 4);
  return ntohl (net_ip);
}

/* -------------------------------------------
 * Route loading
 * ファイル形式: "<cidr> <next-hop-ip>"
 * 例: "10.0.0.0/8 192.0.2.1"
 * ------------------------------------------- */
static struct fib_tree *
_load_routes (const char *path)
{
  FILE *fp = NULL;
  struct fib_tree *tree = NULL;

  char line[LINE_BUF_SIZE];
  char cidr_buf[IP_BUF_SIZE];
  char nh_buf[IP_BUF_SIZE];

  int plen, added;

  uint8_t cidr_net_u8[4]; /* CIDR(ネットワークオーダ) */
  // uint32_t cidr_host_u32; /* CIDR(ホストオーダ) */
  uint8_t nh_net_u8[4]; /* ネクストホップ(ネットワークオーダ) */
  uint32_t nh_host_u32; /* ネクストホップ(ホストオーダ) */

  printf ("Loading routes from file: %s\n", path);
  fp = fopen (path, "r");
  if (!fp)
    {
      fprintf (stderr, "ERROR: cannot open route file: %s\n", path);
      return NULL;
    }

  tree = fib_new (tree);
  if (!tree)
    {
      fprintf (stderr, "ERROR: fib_new failed\n");
      fclose (fp);
      return NULL;
    }

  added = 0;
  while (fgets (line, sizeof (line), fp))
    {
      if (sscanf (line, "%63s %63s", cidr_buf, nh_buf) != 2)
        {
          fprintf (stderr,
                   "WARN: skip invalid line (need: \"<cidr> <nexthop>\"): %s",
                   line);
          continue;
        }

      plen = inet_net_pton (AF_INET, cidr_buf, &cidr_net_u8,
                            sizeof (cidr_net_u8));
      if (plen < 0)
        {
          fprintf (stderr, "WARN: invalid CIDR \"%s\" (skip)\n", cidr_buf);
          continue;
        }

      if (!inet_pton (AF_INET, nh_buf, &nh_net_u8))
        {
          fprintf (stderr, "WARN: invalid next-hop \"%s\" (skip)\n", nh_buf);
          continue;
        }
      nh_host_u32 = ipv4_bytes_to_uint32_ntoh (nh_net_u8);

      if (fib_route_add (tree, cidr_net_u8, plen,
                         (void *)(uintptr_t)nh_host_u32)
          < 0)
        {
          fprintf (stderr, "ERROR: fib_route_add failed for %s %s\n", cidr_buf,
                   nh_buf);
          fib_free (tree);
          fclose (fp);
          return NULL;
        }

      added++;
    }

  printf ("Total %d routes added\n", added);
  fclose (fp);
  return tree;
}

/* -------------------------------------------
 * Basic lookup test
 * ファイル形式: "<ip>"
 * 例: "203.0.113.5"
 * ------------------------------------------- */
int
_run_basic_lookup (struct fib_tree *tree, const char *path)
{
  printf ("============================================\n");

  FILE *fp;
  struct fib_node *node;

  char line[LINE_BUF_SIZE];
  char ip_addr_buf[IP_BUF_SIZE];
  char nh_buf[IP_BUF_SIZE];

  uint8_t ip_addr_net_u8[4]; /* CIDR(ネットワークオーダ) */
  // uint32_t ip_addr_host_u32; /* CIDR(ホストオーダ) */
  uint8_t nh_net_u8[4]; /* ネクストホップ(ネットワークオーダ) */
  uint32_t nh_host_u32; /* ネクストホップ(ホストオーダ) */

  if (!tree || !path)
    return -1;

  printf ("Lookup test with file: %s\n", path);
  fp = fopen (path, "r");
  if (!fp)
    {
      fprintf (stderr, "ERROR: cannot open lookup file: %s\n", path);
      return -1;
    }

  while (fgets (line, sizeof (line), fp))
    {
      if (sscanf (line, "%63s", ip_addr_buf) != 1)
        {
          fprintf (stderr, "WARN: skip invalid line: %s", line);
          continue;
        }

      if (!inet_pton (AF_INET, ip_addr_buf, ip_addr_net_u8))
        {
          fprintf (stderr, "WARN: invalid IPv4 address \"%s\" (skip)\n",
                   ip_addr_buf);
          continue;
        }

      node = fib_route_lookup (tree, ip_addr_net_u8);
      if (node)
        {
          nh_host_u32 = (uint32_t)(uintptr_t)node->data;
          uint32_to_ipv4_bytes_hton (nh_host_u32, nh_net_u8);

          inet_ntop (AF_INET, nh_net_u8, nh_buf, sizeof (nh_buf));
          printf ("+ Found route for %-16s: %s\n", ip_addr_buf, nh_buf);
        }
      else
        printf ("- No route for %s\n", ip_addr_buf);
    }

  fclose (fp);
  return 0;
}

/* -------------------------------------------
 * Performance benchmark
 * ランダム IPv4 を大量に引いてルックアップ（正否は不問）
 * ------------------------------------------- */
int
_benchmark_lookup_performance (struct fib_tree *tree, uint64_t trials)
{
  struct fib_node *n;

  double t1, t2;
  double elapsed, qps;

  uint8_t rand_net_u8[4]; /* CIDR(ネットワークオーダ) */
  uint32_t rand_host_u32; /* CIDR(ホストオーダ) */

  if (!tree || trials == 0)
    return -1;

  t1 = now_seconds ();

  /* 最適化回避用の集計変数 */
  uintptr_t sink = 0;

  for (uint64_t i = 0; i < trials; i++)
    {
      rand_host_u32 = xorshift32 (); /* ホストオーダの乱数 */
      uint32_to_ipv4_bytes_hton (rand_host_u32, rand_net_u8);

      n = fib_route_lookup (tree, rand_net_u8);
      sink ^= (uintptr_t)n;
    }

  t2 = now_seconds ();
  elapsed = t2 - t1;
  qps = (elapsed > 0.0) ? (double)trials / elapsed : 0.0;

  printf ("Elapsed time: %.6f sec for %" PRIu64 " lookups\n", elapsed, trials);
  printf ("Lookup per second: %.6fM lookups/sec\n", qps / 1e6);

  (void)sink; /* 未使用警告抑止 */

  return 0;
}

/* -------------------------------------------
 * Wrapper functions for test.h
 * ------------------------------------------- */
struct fib_tree *
test_load_routes (const char *routes_filename)
{
  return _load_routes (routes_filename);
}

int
test_basic (struct fib_tree *t, const char *lookup_addrs_filename)
{
  return _run_basic_lookup (t, lookup_addrs_filename);
}

int
test_performance (struct fib_tree *t)
{
  const uint64_t trials = 0x10000000ULL;
  return _benchmark_lookup_performance (t, trials);
}