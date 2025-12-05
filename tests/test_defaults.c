#define HT_IMPLEMENTATION
#include "../hashtable.h"
#include <assert.h>
#include <stdio.h>

// Define a hash table for int keys (stored as pointers)
// We will use the built-in ht_hash_int and ht_compare_int
HT_DEFINE(int_ht, void *, int)
HT_IMPLEMENT(int_ht, void *, int)

// Define a hash table for pointer keys (identity)
// We will use the default NULL behavior
HT_DEFINE(ptr_ht, void *, int)
HT_IMPLEMENT(ptr_ht, void *, int)

int main() {
  printf("Testing built-in hash functions...\n");

  // 1. Test explicit built-in int functions
  printf("- Testing ht_hash_int...\n");
  int_ht_t *iht =
      int_ht_create(ht_hash_int, ht_compare_int, NULL, NULL, NULL, NULL);
  assert(iht != NULL);

  int_ht_set(iht, (void *)(uintptr_t)42, 100);
  int_ht_set(iht, (void *)(uintptr_t)10, 200);

  assert(int_ht_get(iht, (void *)(uintptr_t)42) == 100);
  assert(int_ht_get(iht, (void *)(uintptr_t)10) == 200);
  assert(int_ht_get(iht, (void *)(uintptr_t)99) == 0);

  int_ht_destroy(iht);
  printf("  OK\n");

  // 2. Test implicit default (NULL) -> ht_hash_ptr
  printf("- Testing default (NULL) hash (Identity)...\n");
  ptr_ht_t *pht = ptr_ht_create(NULL, NULL, NULL, NULL, NULL, NULL);
  assert(pht != NULL);

  int a = 1;
  int b = 2;

  ptr_ht_set(pht, &a, 10);
  ptr_ht_set(pht, &b, 20);

  assert(ptr_ht_get(pht, &a) == 10);
  assert(ptr_ht_get(pht, &b) == 20);

  // Identity check: different address, same value -> should NOT match
  int c = 1; // Same value as a, but different address
  assert(ptr_ht_get(pht, &c) == 0);

  ptr_ht_destroy(pht);
  printf("  OK\n");

  printf("All built-in tests passed.\n");
  return 0;
}
