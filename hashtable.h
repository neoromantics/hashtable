/**
 * @file hashtable.h
 * @brief A generic, header-only hash table implementation in C.
 *
 * # Architecture & Design
 *
 * ## 1. Open Addressing with Linear Probing
 * This hash table uses **Open Addressing** for collision resolution,
 * specifically **Linear Probing**.
 * - **Open Addressing**: All elements are stored directly in the `entries`
 * array. There are no linked lists or external nodes.
 * - **Linear Probing**: When a collision occurs (two keys hash to the same
 * index), the algorithm checks the next slot (index + 1), wrapping around if
 * necessary, until an empty slot is found.
 *
 * ## 2. Memory Layout
 * The table consists of a single contiguous array of `entry_t` structs.
 * Each entry contains:
 * - `key`: The key (type defined by user).
 * - `value`: The value (type defined by user).
 * - `state`: The state of the slot (EMPTY, OCCUPIED, or DELETED).
 *
 * This layout improves cache locality compared to chaining (linked lists)
 * because accessing the next probe location involves reading the next memory
 * address, which is likely already in the CPU cache.
 *
 * ## 3. Tombstones (Lazy Deletion)
 * Deletions are handled using "tombstones" (lazy deletion).
 * - When an item is deleted, its slot state is marked as `HT_SLOT_DELETED`
 * instead of `HT_SLOT_EMPTY`.
 * - **Search**: The search algorithm continues past `HT_SLOT_DELETED` slots,
 * ensuring that items placed *after* the deleted item (due to probing) can
 * still be found.
 * - **Insertion**: New items can overwrite `HT_SLOT_DELETED` slots, reclaiming
 * the space.
 * - **Cleanup**: Tombstones are permanently removed during a resize (rehash)
 * operation.
 *
 * ## 4. Thread Safety
 * Thread safety is achieved using a `pthread_rwlock_t` (Read-Write Lock).
 * - **Readers**: Multiple threads can read (`get`) simultaneously.
 * - **Writers**: Only one thread can write (`set`, `delete`, `clear`,
 * `reserve`) at a time.
 * - **Granularity**: The lock covers the entire table. This is simple and
 * effective for read-heavy workloads but may be a bottleneck for write-heavy
 * concurrent scenarios.
 *
 * ## 5. Type Safety via Macros
 * The library uses C preprocessor macros (`HT_DEFINE`, `HT_IMPLEMENT`) to
 * generate type-safe code for specific key/value pairs.
 * - `HT_DEFINE(name, key_type, value_type)`: Defines the structs and function
 * prototypes.
 * - `HT_IMPLEMENT(name, key_type, value_type)`: Generates the actual function
 * implementations.
 *
 * ## 6. Hashing
 * The default hash function for strings uses **MurmurHash3**, a fast,
 * non-cryptographic hash function with excellent distribution properties.
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  HT_SUCCESS,
  HT_ERROR_MEMORY,
} ht_status;

uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed);

/**
 * @brief Default hash function for strings.
 * Uses MurmurHash3.
 * @param key The string key.
 * @param user_data User data (unused).
 * @return The hash value.
 */
static inline uint32_t ht_hash_string(const void *key, void *user_data) {
  (void)user_data;
  return murmur3_32((const uint8_t *)key, strlen((const char *)key), 0);
}

/**
 * @brief Default comparison function for strings.
 * @param key1 First key.
 * @param key2 Second key.
 * @param user_data User data (unused).
 * @return 1 if equal, 0 otherwise.
 */
static inline int ht_compare_string(const void *key1, const void *key2,
                                    void *user_data) {
  (void)user_data;
  return strcmp((const char *)key1, (const char *)key2) == 0;
}

/**
 * @brief Default free function for strings (or any malloc'd pointer).
 * @param ptr Pointer to free.
 * @param user_data User data (unused).
 */
static inline void ht_free_string(void *ptr, void *user_data) {
  (void)user_data;
  free(ptr);
}

/**
 * @brief Hash function for integers (cast to void*).
 */
static inline uint32_t ht_hash_int(const void *key, void *user_data) {
  (void)user_data;
  uint32_t v = (uint32_t)(uintptr_t)key;
  return murmur3_32((const uint8_t *)&v, sizeof(uint32_t), 0);
}

/**
 * @brief Compare function for integers (cast to void*).
 */
static inline int ht_compare_int(const void *key1, const void *key2,
                                 void *user_data) {
  (void)user_data;
  return (int)(uintptr_t)key1 == (int)(uintptr_t)key2;
}

/**
 * @brief Hash function for pointers (identity hash).
 * Hashes the address itself.
 */
static inline uint32_t ht_hash_ptr(const void *key, void *user_data) {
  (void)user_data;
  return murmur3_32((const uint8_t *)&key, sizeof(void *), 0);
}

/**
 * @brief Compare function for pointers (identity comparison).
 */
static inline int ht_compare_ptr(const void *key1, const void *key2,
                                 void *user_data) {
  (void)user_data;
  return key1 == key2;
}

/**
 * @brief State of a slot in the hash table.
 * - EMPTY: Slot is free and has never been used (stops search).
 * - OCCUPIED: Slot contains a valid key-value pair.
 * - DELETED: Slot was used but is now free (tombstone; does not stop search).
 */
typedef enum {
  HT_SLOT_EMPTY = 0,
  HT_SLOT_OCCUPIED,
  HT_SLOT_DELETED
} ht_slot_state;

/**
 * @brief Macro to define a type-safe hash table.
 *
 * @param name The prefix for the generated types and functions (e.g., `my_ht`
 * -> `my_ht_t`, `my_ht_create`).
 * @param key_type The type of the keys (e.g., `int`, `char*`, `MyStruct*`).
 * @param value_type The type of the values (e.g., `int`, `MyData*`).
 */
#define HT_DEFINE(name, key_type, value_type)                                  \
  typedef struct {                                                             \
    key_type key;                                                              \
    value_type value;                                                          \
    ht_slot_state state;                                                       \
  } name##_entry_t;                                                            \
                                                                               \
  typedef struct name {                                                        \
    name##_entry_t *entries;                                                   \
    size_t capacity;                                                           \
    size_t size;                                                               \
    size_t deleted_count;                                                      \
    pthread_rwlock_t rwlock;                                                   \
    uint32_t (*hash)(const void *key, void *user_data);                        \
    int (*compare)(const void *key1, const void *key2, void *user_data);       \
    void (*key_free)(void *key, void *user_data);                              \
    void (*value_free)(void *value, void *user_data);                          \
    value_type (*value_copy)(const value_type value, void *user_data);         \
    void *user_data;                                                           \
  } name##_t;                                                                  \
                                                                               \
  /**                                                                          \
   * @brief Creates a new hash table.                                          \
   * @param hash Hash function.                                                \
   * @param compare Key comparison function.                                   \
   * @param key_free Function to free keys (optional).                         \
   * @param value_free Function to free values (optional).                     \
   * @param value_copy Function to copy values (optional, used in get).        \
   * @param user_data User data passed to callbacks.                           \
   * @return Pointer to the new hash table, or NULL on failure.                \
   */                                                                          \
  name##_t *name##_create(                                                     \
      uint32_t (*hash)(const void *key, void *user_data),                      \
      int (*compare)(const void *key1, const void *key2, void *user_data),     \
      void (*key_free)(void *key, void *user_data),                            \
      void (*value_free)(void *value, void *user_data),                        \
      value_type (*value_copy)(const value_type value, void *user_data),       \
      void *user_data);                                                        \
                                                                               \
  /**                                                                          \
   * @brief Destroys the hash table and frees all memory.                      \
   * @param ht The hash table.                                                 \
   */                                                                          \
  void name##_destroy(name##_t *ht);                                           \
                                                                               \
  /**                                                                          \
   * @brief Inserts or updates a key-value pair.                               \
   * @param ht The hash table.                                                 \
   * @param key The key.                                                       \
   * @param value The value.                                                   \
   * @return HT_SUCCESS on success, HT_ERROR_MEMORY on failure.                \
   */                                                                          \
  ht_status name##_set(name##_t *ht, key_type key, value_type value);          \
                                                                               \
  /**                                                                          \
   * @brief Retrieves a value by key.                                          \
   * @param ht The hash table.                                                 \
   * @param key The key.                                                       \
   * @return The value, or 0/NULL if not found.                                \
   */                                                                          \
  value_type name##_get(name##_t *ht, key_type key);                           \
                                                                               \
  /**                                                                          \
   * @brief Deletes a key-value pair.                                          \
   * @param ht The hash table.                                                 \
   * @param key The key.                                                       \
   * @return 1 if deleted, 0 if not found.                                     \
   */                                                                          \
  int name##_delete(name##_t *ht, key_type key);                               \
                                                                               \
  /**                                                                          \
   * @brief Clears all items from the hash table but keeps capacity.           \
   * @param ht The hash table.                                                 \
   */                                                                          \
  void name##_clear(name##_t *ht);                                             \
                                                                               \
  /**                                                                          \
   * @brief Reserves capacity for the hash table.                              \
   * @param ht The hash table.                                                 \
   * @param capacity The desired capacity.                                     \
   * @return HT_SUCCESS on success, HT_ERROR_MEMORY on failure.                \
   */                                                                          \
  ht_status name##_reserve(name##_t *ht, size_t capacity);                     \
                                                                               \
  typedef struct name##_iter {                                                 \
    name##_t *ht;                                                              \
    size_t index;                                                              \
  } name##_iter_t;                                                             \
                                                                               \
  /**                                                                          \
   * @brief Creates an iterator for the hash table.                            \
   * @param ht The hash table.                                                 \
   * @return The iterator.                                                     \
   */                                                                          \
  name##_iter_t name##_iter_create(name##_t *ht);                              \
                                                                               \
  /**                                                                          \
   * @brief Advances the iterator to the next item.                            \
   * @param iter The iterator.                                                 \
   * @param key Pointer to store the key (optional).                           \
   * @param value Pointer to store the value (optional).                       \
   * @return 1 if valid item found, 0 if end of table.                         \
   */                                                                          \
  int name##_iter_next(name##_iter_t *iter, key_type *key, value_type *value);

#ifdef HT_IMPLEMENTATION

#define HT_IMPLEMENT(name, key_type, value_type)                               \
  name##_t *name##_create(                                                     \
      uint32_t (*hash)(const void *key, void *user_data),                      \
      int (*compare)(const void *key1, const void *key2, void *user_data),     \
      void (*key_free)(void *key, void *user_data),                            \
      void (*value_free)(void *value, void *user_data),                        \
      value_type (*value_copy)(const value_type value, void *user_data),       \
      void *user_data) {                                                       \
    name##_t *ht = (name##_t *)calloc(1, sizeof(name##_t));                    \
    if (!ht)                                                                   \
      return NULL;                                                             \
    ht->capacity = 16;                                                         \
    ht->entries =                                                              \
        (name##_entry_t *)calloc(ht->capacity, sizeof(name##_entry_t));        \
    if (!ht->entries) {                                                        \
      free(ht);                                                                \
      return NULL;                                                             \
    }                                                                          \
    pthread_rwlock_init(&ht->rwlock, NULL);                                    \
    ht->hash = hash ? hash : ht_hash_ptr;                                      \
    ht->compare = compare ? compare : ht_compare_ptr;                          \
    ht->key_free = key_free;                                                   \
    ht->value_free = value_free;                                               \
    ht->value_copy = value_copy;                                               \
    ht->user_data = user_data;                                                 \
    return ht;                                                                 \
  }                                                                            \
                                                                               \
  void name##_destroy(name##_t *ht) {                                          \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    for (size_t i = 0; i < ht->capacity; i++) {                                \
      if (ht->entries[i].state == HT_SLOT_OCCUPIED) {                          \
        if (ht->key_free)                                                      \
          ht->key_free((void *)(uintptr_t)ht->entries[i].key, ht->user_data);  \
        if (ht->value_free)                                                    \
          ht->value_free((void *)(uintptr_t)ht->entries[i].value,              \
                         ht->user_data);                                       \
      }                                                                        \
    }                                                                          \
    free(ht->entries);                                                         \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    pthread_rwlock_destroy(&ht->rwlock);                                       \
    free(ht);                                                                  \
  }                                                                            \
                                                                               \
  static ht_status name##_resize(name##_t *ht, size_t new_capacity) {          \
    name##_entry_t *old_entries = ht->entries;                                 \
    size_t old_capacity = ht->capacity;                                        \
                                                                               \
    name##_entry_t *new_entries =                                              \
        (name##_entry_t *)calloc(new_capacity, sizeof(name##_entry_t));        \
    if (!new_entries)                                                          \
      return HT_ERROR_MEMORY;                                                  \
                                                                               \
    ht->entries = new_entries;                                                 \
    ht->capacity = new_capacity;                                               \
    ht->size = 0; /* Reset size, will be re-counted during rehash */           \
    ht->deleted_count = 0;                                                     \
                                                                               \
    for (size_t i = 0; i < old_capacity; i++) {                                \
      if (old_entries[i].state == HT_SLOT_OCCUPIED) {                          \
        size_t index =                                                         \
            ht->hash(old_entries[i].key, ht->user_data) % new_capacity;        \
        while (ht->entries[index].state == HT_SLOT_OCCUPIED) {                 \
          index = (index + 1) % new_capacity;                                  \
        }                                                                      \
        ht->entries[index] = old_entries[i];                                   \
        ht->entries[index].state = HT_SLOT_OCCUPIED; /* Ensure state is set */ \
        ht->size++;                                                            \
      }                                                                        \
    }                                                                          \
                                                                               \
    free(old_entries);                                                         \
    return HT_SUCCESS;                                                         \
  }                                                                            \
                                                                               \
  ht_status name##_set(name##_t *ht, key_type key, value_type value) {         \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    /* Resize if load factor is too high or too many deleted slots */          \
    if ((float)(ht->size + ht->deleted_count) / ht->capacity > 0.75 ||         \
        ht->deleted_count > ht->size / 2) {                                    \
      if (name##_resize(ht, ht->capacity * 2) != HT_SUCCESS) {                 \
        pthread_rwlock_unlock(&ht->rwlock);                                    \
        return HT_ERROR_MEMORY;                                                \
      }                                                                        \
    }                                                                          \
                                                                               \
    size_t index = ht->hash(key, ht->user_data) % ht->capacity;                \
    size_t first_deleted = (size_t)-1;                                         \
                                                                               \
    while (ht->entries[index].state != HT_SLOT_EMPTY) {                        \
      if (ht->entries[index].state == HT_SLOT_OCCUPIED) {                      \
        if (ht->compare(ht->entries[index].key, key, ht->user_data)) {         \
          if (ht->value_free)                                                  \
            ht->value_free((void *)(uintptr_t)ht->entries[index].value,        \
                           ht->user_data);                                     \
          ht->entries[index].value = value;                                    \
          pthread_rwlock_unlock(&ht->rwlock);                                  \
          return HT_SUCCESS;                                                   \
        }                                                                      \
      } else if (first_deleted == (size_t)-1) { /* HT_SLOT_DELETED */          \
        first_deleted = index;                                                 \
      }                                                                        \
      index = (index + 1) % ht->capacity;                                      \
    }                                                                          \
                                                                               \
    /* If a deleted slot was found, use it */                                  \
    if (first_deleted != (size_t)-1) {                                         \
      index = first_deleted;                                                   \
      ht->deleted_count--;                                                     \
    }                                                                          \
                                                                               \
    ht->entries[index].key = key;                                              \
    ht->entries[index].value = value;                                          \
    ht->entries[index].state = HT_SLOT_OCCUPIED;                               \
    ht->size++;                                                                \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return HT_SUCCESS;                                                         \
  }                                                                            \
                                                                               \
  value_type name##_get(name##_t *ht, key_type key) {                          \
    pthread_rwlock_rdlock(&ht->rwlock);                                        \
    size_t index = ht->hash(key, ht->user_data) % ht->capacity;                \
                                                                               \
    while (ht->entries[index].state != HT_SLOT_EMPTY) {                        \
      if (ht->entries[index].state == HT_SLOT_OCCUPIED) {                      \
        if (ht->compare(ht->entries[index].key, key, ht->user_data)) {         \
          value_type val =                                                     \
              ht->value_copy                                                   \
                  ? ht->value_copy(ht->entries[index].value, ht->user_data)    \
                  : ht->entries[index].value;                                  \
          pthread_rwlock_unlock(&ht->rwlock);                                  \
          return val;                                                          \
        }                                                                      \
      }                                                                        \
      index = (index + 1) % ht->capacity;                                      \
    }                                                                          \
                                                                               \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return (value_type)0;                                                      \
  }                                                                            \
                                                                               \
  int name##_delete(name##_t *ht, key_type key) {                              \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    size_t index = ht->hash(key, ht->user_data) % ht->capacity;                \
                                                                               \
    while (ht->entries[index].state != HT_SLOT_EMPTY) {                        \
      if (ht->entries[index].state == HT_SLOT_OCCUPIED) {                      \
        if (ht->compare(ht->entries[index].key, key, ht->user_data)) {         \
          if (ht->key_free)                                                    \
            ht->key_free((void *)(uintptr_t)ht->entries[index].key,            \
                         ht->user_data);                                       \
          if (ht->value_free)                                                  \
            ht->value_free((void *)(uintptr_t)ht->entries[index].value,        \
                           ht->user_data);                                     \
          ht->entries[index].state = HT_SLOT_DELETED;                          \
          ht->size--;                                                          \
          ht->deleted_count++;                                                 \
          pthread_rwlock_unlock(&ht->rwlock);                                  \
          return 1;                                                            \
        }                                                                      \
      }                                                                        \
      index = (index + 1) % ht->capacity;                                      \
    }                                                                          \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return 0;                                                                  \
  }                                                                            \
                                                                               \
  void name##_clear(name##_t *ht) {                                            \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    for (size_t i = 0; i < ht->capacity; i++) {                                \
      if (ht->entries[i].state == HT_SLOT_OCCUPIED) {                          \
        if (ht->key_free)                                                      \
          ht->key_free((void *)(uintptr_t)ht->entries[i].key, ht->user_data);  \
        if (ht->value_free)                                                    \
          ht->value_free((void *)(uintptr_t)ht->entries[i].value,              \
                         ht->user_data);                                       \
      }                                                                        \
      ht->entries[i].state = HT_SLOT_EMPTY;                                    \
    }                                                                          \
    ht->size = 0;                                                              \
    ht->deleted_count = 0;                                                     \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
  }                                                                            \
                                                                               \
  ht_status name##_reserve(name##_t *ht, size_t capacity) {                    \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    if (capacity <= ht->capacity) {                                            \
      pthread_rwlock_unlock(&ht->rwlock);                                      \
      return HT_SUCCESS;                                                       \
    }                                                                          \
    /* Ensure new capacity is at least current size + deleted_count to avoid   \
     * immediate rehash */                                                     \
    if (capacity < ht->size + ht->deleted_count) {                             \
      capacity = ht->size + ht->deleted_count;                                 \
    }                                                                          \
    /* Round up to next power of 2 for better performance, or just use as is   \
     */                                                                        \
    /* For simplicity, let's just use the provided capacity, but ensure it's   \
     * large enough */                                                         \
    ht_status status = name##_resize(ht, capacity);                            \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return status;                                                             \
  }                                                                            \
                                                                               \
  name##_iter_t name##_iter_create(name##_t *ht) {                             \
    name##_iter_t iter;                                                        \
    iter.ht = ht;                                                              \
    iter.index = 0;                                                            \
    return iter;                                                               \
  }                                                                            \
                                                                               \
  int name##_iter_next(name##_iter_t *iter, key_type *key,                     \
                       value_type *value) {                                    \
    while (iter->index < iter->ht->capacity) {                                 \
      if (iter->ht->entries[iter->index].state == HT_SLOT_OCCUPIED) {          \
        if (key)                                                               \
          *key = iter->ht->entries[iter->index].key;                           \
        if (value)                                                             \
          *value = iter->ht->entries[iter->index].value;                       \
        iter->index++;                                                         \
        return 1;                                                              \
      }                                                                        \
      iter->index++;                                                           \
    }                                                                          \
    return 0;                                                                  \
  }

uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed) {
  uint32_t h = seed;
  if (len > 3) {
    const uint32_t *key_x4 = (const uint32_t *)key;
    size_t i = len >> 2;
    do {
      uint32_t k = *key_x4++;
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
      h = (h << 13) | (h >> 19);
      h = h * 5 + 0xe6546b64;
    } while (--i);
    key = (const uint8_t *)key_x4;
  }
  if (len & 3) {
    size_t i = len & 3;
    uint32_t k = 0;
    key = &key[i - 1];
    do {
      k <<= 8;
      k |= *key--;
    } while (--i);
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
  }
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

#endif // HT_IMPLEMENTATION

#endif // HASHTABLE_H