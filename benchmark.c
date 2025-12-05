#define HT_IMPLEMENTATION
#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Simple timer
double get_time() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Define a hash table for string keys and int values
HT_DEFINE(bench_ht, char *, int)
HT_IMPLEMENT(bench_ht, char *, int)

#define NUM_ITEMS 1000000

int main() {
  printf("Benchmarking with %d items...\n", NUM_ITEMS);

  // Create keys
  char **keys = malloc(NUM_ITEMS * sizeof(char *));
  for (int i = 0; i < NUM_ITEMS; i++) {
    char buf[32];
    sprintf(buf, "key-%d", i);
    keys[i] = strdup(buf);
  }

  bench_ht_t *ht = bench_ht_create(ht_hash_string, ht_compare_string, NULL,
                                   NULL, NULL, NULL);

  // Benchmark Insertion
  double start = get_time();
  for (int i = 0; i < NUM_ITEMS; i++) {
    bench_ht_set(ht, keys[i], i);
  }
  double end = get_time();
  printf("Insertion: %.4f seconds (%.2f ops/sec)\n", end - start,
         NUM_ITEMS / (end - start));

  // Benchmark Lookup (Hit)
  start = get_time();
  for (int i = 0; i < NUM_ITEMS; i++) {
    volatile int val = bench_ht_get(ht, keys[i]);
    (void)val;
  }
  end = get_time();
  printf("Lookup (Hit): %.4f seconds (%.2f ops/sec)\n", end - start,
         NUM_ITEMS / (end - start));

  // Benchmark Lookup (Miss)
  start = get_time();
  for (int i = 0; i < NUM_ITEMS; i++) {
    volatile int val = bench_ht_get(ht, "non-existent");
    (void)val;
  }
  end = get_time();
  printf("Lookup (Miss): %.4f seconds (%.2f ops/sec)\n", end - start,
         NUM_ITEMS / (end - start));

  // Benchmark Deletion
  start = get_time();
  for (int i = 0; i < NUM_ITEMS; i++) {
    bench_ht_delete(ht, keys[i]);
  }
  end = get_time();
  printf("Deletion: %.4f seconds (%.2f ops/sec)\n", end - start,
         NUM_ITEMS / (end - start));

  // Cleanup
  bench_ht_destroy(ht);
  for (int i = 0; i < NUM_ITEMS; i++) {
    free(keys[i]);
  }
  free(keys);

  return 0;
}
