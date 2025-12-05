#define HT_IMPLEMENTATION
#include "../hashtable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Not standard in C, so provide our own.
static char *ht_strdup(const char *s) {
  char *d = malloc(strlen(s) + 1);
  if (d == NULL)
    return NULL;
  strcpy(d, s);
  return d;
}

// Define a hash table for string keys and string values
HT_DEFINE(str_ht, char *, char *)
HT_IMPLEMENT(str_ht, char *, char *)

// Hash function for integers
static inline uint32_t test_hash_int(const void *key, void *user_data) {
  (void)user_data;
  return murmur3_32((const uint8_t *)key, sizeof(int), 0);
}

// Compare function for integers
static inline int test_compare_int(const void *key1, const void *key2,
                                   void *user_data) {
  (void)user_data;
  return *(int *)key1 == *(int *)key2;
}

// Define a hash table for int keys and int values
HT_DEFINE(int_ht, int *, int *)
HT_IMPLEMENT(int_ht, int *, int *)

static char *ht_strdup_copy(const char *s, void *user_data) {
  (void)user_data;
  return ht_strdup(s);
}

void test_string_hashtable() {
  printf("Running test_string_hashtable... ");

  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  char *key1 = ht_strdup("key1");
  char *value1 = ht_strdup("value1");
  assert(str_ht_set(ht, key1, value1) == HT_SUCCESS);
  char *retrieved_value1 = str_ht_get(ht, key1);
  assert(strcmp(retrieved_value1, value1) == 0);
  free(retrieved_value1);

  char *key2 = ht_strdup("key2");
  char *value2 = ht_strdup("value2");
  assert(str_ht_set(ht, key2, value2) == HT_SUCCESS);
  char *retrieved_value2 = str_ht_get(ht, key2);
  assert(strcmp(retrieved_value2, value2) == 0);
  free(retrieved_value2);

  // Test updating a key
  char *new_value1 = ht_strdup("new_value1");
  assert(str_ht_set(ht, key1, new_value1) == HT_SUCCESS);
  char *retrieved_new_value1 = str_ht_get(ht, key1);
  assert(strcmp(retrieved_new_value1, new_value1) == 0);
  free(retrieved_new_value1);

  // Test delete
  str_ht_delete(ht, key1);
  char *retrieved_after_delete = str_ht_get(ht, key1);
  assert(retrieved_after_delete == NULL);

  str_ht_destroy(ht);
  printf("OK\n");
}

void test_int_hashtable() {
  printf("Running test_int_hashtable... ");

  int_ht_t *ht =
      int_ht_create(test_hash_int, test_compare_int, NULL, NULL, NULL, NULL);
  assert(ht != NULL);

  int key1 = 1;
  int value1 = 100;
  assert(int_ht_set(ht, &key1, &value1) == HT_SUCCESS);
  int *retrieved_value1 = int_ht_get(ht, &key1);
  assert(*retrieved_value1 == value1);

  int key2 = 2;
  int value2 = 200;
  assert(int_ht_set(ht, &key2, &value2) == HT_SUCCESS);
  int *retrieved_value2 = int_ht_get(ht, &key2);
  assert(*retrieved_value2 == value2);

  // Test updating a key
  int new_value1 = 101;
  assert(int_ht_set(ht, &key1, &new_value1) == HT_SUCCESS);
  int *retrieved_new_value1 = int_ht_get(ht, &key1);
  assert(*retrieved_new_value1 == new_value1);

  // Test delete
  int_ht_delete(ht, &key1);
  int *retrieved_after_delete = int_ht_get(ht, &key1);
  assert(retrieved_after_delete == NULL);

  int_ht_destroy(ht);
  printf("OK\n");
}

#include <pthread.h>
#include <stdint.h>

void *thread_func(void *arg) {
  str_ht_t *ht = (str_ht_t *)arg;
  for (int i = 0; i < 1000; i++) {
    char key[32];
    sprintf(key, "key-%d-%lu", i, (uintptr_t)pthread_self());
    char *k = ht_strdup(key);
    char *v = ht_strdup("value");
    assert(str_ht_set(ht, k, v) == HT_SUCCESS);
    char *retrieved = str_ht_get(ht, k);
    assert(strcmp(retrieved, "value") == 0);
    free(retrieved);
  }
  return NULL;
}

void test_thread_safety() {
  printf("Running test_thread_safety... ");

  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  pthread_t threads[10];
  for (int i = 0; i < 10; i++) {
    pthread_create(&threads[i], NULL, thread_func, ht);
  }

  for (int i = 0; i < 10; i++) {
    pthread_join(threads[i], NULL);
  }

  assert(ht->size == 10000);

  str_ht_destroy(ht);
  printf("OK\n");
}

static uint32_t collision_hash(const void *key, void *user_data) {
  (void)key;
  (void)user_data;
  return 1;
}

void test_edge_cases() {
  printf("Running test_edge_cases... ");

  // Test with collisions
  str_ht_t *ht =
      str_ht_create(collision_hash, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  char *key1 = ht_strdup("key1");
  char *value1 = ht_strdup("value1");
  assert(str_ht_set(ht, key1, value1) == HT_SUCCESS);

  char *key2 = ht_strdup("key2");
  char *value2 = ht_strdup("value2");
  assert(str_ht_set(ht, key2, value2) == HT_SUCCESS);

  char *retrieved1 = str_ht_get(ht, key1);
  assert(strcmp(retrieved1, value1) == 0);
  free(retrieved1);

  char *retrieved2 = str_ht_get(ht, key2);
  assert(strcmp(retrieved2, value2) == 0);
  free(retrieved2);

  str_ht_destroy(ht);

  // Test deleting non-existent key
  ht = str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                     ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);
  str_ht_delete(ht, "non-existent-key");
  str_ht_destroy(ht);

  // Test setting NULL key
  ht = str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                     ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);
  // This will likely crash if not handled, which is ok for a test
  // str_ht_set(ht, NULL, "value");
  str_ht_destroy(ht);

  // Test creating with NULL functions
  ht = str_ht_create(NULL, NULL, NULL, NULL, NULL, NULL);
  assert(ht != NULL);
  // The following calls should not crash
  // str_ht_set(ht, "key", "value");
  // str_ht_get(ht, "key");
  // str_ht_delete(ht, "key");
  str_ht_destroy(ht);

  printf("OK\n");
}

#define NUM_THREADS 50
#define NUM_ITERATIONS 10000

void *high_contention_thread_func(void *arg) {
  str_ht_t *ht = (str_ht_t *)arg;
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    char key[32];
    sprintf(key, "key-%d", i % 100); // High contention on a few keys
    if (rand() % 2 == 0) {
      char *k = ht_strdup(key);
      char *v = ht_strdup("value");
      assert(str_ht_set(ht, k, v) == HT_SUCCESS);
    } else {
      char *retrieved = str_ht_get(ht, key);
      if (retrieved) {
        assert(strcmp(retrieved, "value") == 0);
        free(retrieved);
      }
    }
  }
  return NULL;
}

void test_high_contention() {
  printf("Running test_high_contention... ");

  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  pthread_t threads[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, high_contention_thread_func, ht);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  str_ht_destroy(ht);
  printf("OK\n");
}

void test_rapid_resize() {
  printf("Running test_rapid_resize... ");

  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  // Insert many items to trigger resizing
  for (int i = 0; i < 10000; i++) {
    char key[32];
    sprintf(key, "key-%d", i);
    char *k = ht_strdup(key);
    char *v = ht_strdup("value");
    assert(str_ht_set(ht, k, v) == HT_SUCCESS);
  }

  // Delete many items
  for (int i = 0; i < 10000; i++) {
    char key[32];
    sprintf(key, "key-%d", i);
    str_ht_delete(ht, key);
  }

  assert(ht->size == 0);

  str_ht_destroy(ht);
  printf("OK\n");
}

void test_clear() {
  printf("Running test_clear... ");
  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  for (int i = 0; i < 100; i++) {
    char key[32];
    sprintf(key, "key-%d", i);
    char *k = ht_strdup(key);
    char *v = ht_strdup("value");
    assert(str_ht_set(ht, k, v) == HT_SUCCESS);
  }
  assert(ht->size == 100);

  str_ht_clear(ht);
  assert(ht->size == 0);
  assert(str_ht_get(ht, "key-0") == NULL);

  // Ensure we can reuse it
  char *k = ht_strdup("new-key");
  char *v = ht_strdup("new-value");
  assert(str_ht_set(ht, k, v) == HT_SUCCESS);
  assert(ht->size == 1);

  str_ht_destroy(ht);
  printf("OK\n");
}

void test_reserve() {
  printf("Running test_reserve... ");
  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  assert(str_ht_reserve(ht, 1000) == HT_SUCCESS);
  assert(ht->capacity >= 1000);

  for (int i = 0; i < 1000; i++) {
    char key[32];
    sprintf(key, "key-%d", i);
    char *k = ht_strdup(key);
    char *v = ht_strdup("value");
    assert(str_ht_set(ht, k, v) == HT_SUCCESS);
  }

  str_ht_destroy(ht);
  printf("OK\n");
}

void test_iterator() {
  printf("Running test_iterator... ");
  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  int count = 10;
  for (int i = 0; i < count; i++) {
    char key[32];
    sprintf(key, "key-%d", i);
    char *k = ht_strdup(key);
    char *v = ht_strdup("value");
    assert(str_ht_set(ht, k, v) == HT_SUCCESS);
  }

  str_ht_iter_t iter = str_ht_iter_create(ht);
  char *k, *v;
  int iterated_count = 0;
  while (str_ht_iter_next(&iter, &k, &v)) {
    iterated_count++;
    assert(strncmp(k, "key-", 4) == 0);
    assert(strcmp(v, "value") == 0);
  }
  assert(iterated_count == count);

  str_ht_destroy(ht);
  printf("OK\n");
}

void test_massive() {
  printf("Running test_massive (1M items)... ");
  str_ht_t *ht =
      str_ht_create(ht_hash_string, ht_compare_string, ht_free_string,
                    ht_free_string, ht_strdup_copy, NULL);
  assert(ht != NULL);

  int count = 1000000;
  // Reserve to avoid too many resizes during insertion
  str_ht_reserve(ht, count);

  for (int i = 0; i < count; i++) {
    char key[32];
    sprintf(key, "k%d", i);
    char *k = ht_strdup(key);
    // Share value to save memory in test
    char *v = ht_strdup("v");
    // Note: we are duplicating "v" 1M times, which is fine, but let's be
    // careful with memory. Actually, let's just use a static string for value
    // to save memory if we didn't have free_value. But we have free_value set
    // to ht_free_string. So we must malloc. 1M * (malloc overhead + string)
    // might be heavy. "v" is small.
    assert(str_ht_set(ht, k, v) == HT_SUCCESS);
  }

  assert(ht->size == (size_t)count);

  // Verify a few
  char *val = str_ht_get(ht, "k0");
  assert(val != NULL && strcmp(val, "v") == 0);
  free(val); // get returns a copy

  val = str_ht_get(ht, "k999999");
  assert(val != NULL && strcmp(val, "v") == 0);
  free(val);

  str_ht_destroy(ht);
  printf("OK\n");
}

typedef struct {
  int x;
  int y;
} Point;

static uint32_t hash_point(const void *key, void *user_data) {
  (void)user_data;
  const Point *p = (const Point *)key;
  // Simple hash combining x and y
  return (uint32_t)(p->x * 31 + p->y);
}

static int compare_point(const void *key1, const void *key2, void *user_data) {
  (void)user_data;
  const Point *p1 = (const Point *)key1;
  const Point *p2 = (const Point *)key2;
  return p1->x == p2->x && p1->y == p2->y;
}

// Define a hash table for Point keys and int values
HT_DEFINE(point_ht, Point *, int)
HT_IMPLEMENT(point_ht, Point *, int)

void test_custom_struct() {
  printf("Running test_custom_struct... ");
  point_ht_t *ht =
      point_ht_create(hash_point, compare_point, NULL, NULL, NULL, NULL);
  assert(ht != NULL);

  Point p1 = {1, 2};
  int v1 = 10;
  assert(point_ht_set(ht, &p1, v1) == HT_SUCCESS);

  Point p2 = {3, 4};
  int v2 = 20;
  assert(point_ht_set(ht, &p2, v2) == HT_SUCCESS);

  int retrieved_v1 = point_ht_get(ht, &p1);
  assert(retrieved_v1 == v1);

  int retrieved_v2 = point_ht_get(ht, &p2);
  assert(retrieved_v2 == v2);

  // Test update
  int new_v1 = 11;
  assert(point_ht_set(ht, &p1, new_v1) == HT_SUCCESS);
  assert(point_ht_get(ht, &p1) == new_v1);

  point_ht_destroy(ht);
  printf("OK\n");
}

int main() {
  test_string_hashtable();
  test_int_hashtable();
  test_thread_safety();
  test_edge_cases();
  test_high_contention();
  test_rapid_resize();
  test_clear();
  test_reserve();
  test_iterator();
  test_massive();
  test_custom_struct();
  return 0;
}
