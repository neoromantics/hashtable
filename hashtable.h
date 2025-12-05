/**
 * @file hashtable.h
 * @brief A generic, header-only hash table implementation in C.
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

#define HT_DEFINE(name, key_type, value_type)                                  \
  typedef struct name##_entry {                                                \
    key_type key;                                                              \
    value_type value;                                                          \
    struct name##_entry *next;                                                 \
  } name##_entry_t;                                                            \
                                                                               \
  typedef struct name {                                                        \
    name##_entry_t **entries;                                                  \
    size_t capacity;                                                           \
    size_t size;                                                               \
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
    name##_entry_t *entry;                                                     \
  } name##_iter_t;                                                             \
                                                                               \
  name##_iter_t name##_iter_create(name##_t *ht);                              \
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
        (name##_entry_t **)calloc(ht->capacity, sizeof(name##_entry_t *));     \
    if (!ht->entries) {                                                        \
      free(ht);                                                                \
      return NULL;                                                             \
    }                                                                          \
    pthread_rwlock_init(&ht->rwlock, NULL);                                    \
    ht->hash = hash;                                                           \
    ht->compare = compare;                                                     \
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
      name##_entry_t *entry = ht->entries[i];                                  \
      while (entry) {                                                          \
        name##_entry_t *next = entry->next;                                    \
        if (ht->key_free)                                                      \
          ht->key_free((void *)(uintptr_t)entry->key, ht->user_data);          \
        if (ht->value_free)                                                    \
          ht->value_free((void *)(uintptr_t)entry->value, ht->user_data);      \
        free(entry);                                                           \
        entry = next;                                                          \
      }                                                                        \
    }                                                                          \
    free(ht->entries);                                                         \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    pthread_rwlock_destroy(&ht->rwlock);                                       \
    free(ht);                                                                  \
  }                                                                            \
                                                                               \
  static ht_status name##_resize(name##_t *ht) {                               \
    size_t new_capacity = ht->capacity * 2;                                    \
    name##_entry_t **new_entries =                                             \
        (name##_entry_t **)calloc(new_capacity, sizeof(name##_entry_t *));     \
    if (!new_entries)                                                          \
      return HT_ERROR_MEMORY;                                                  \
                                                                               \
    for (size_t i = 0; i < ht->capacity; i++) {                                \
      name##_entry_t *entry = ht->entries[i];                                  \
      while (entry) {                                                          \
        name##_entry_t *next = entry->next;                                    \
        size_t new_index = ht->hash(entry->key, ht->user_data) % new_capacity; \
        entry->next = new_entries[new_index];                                  \
        new_entries[new_index] = entry;                                        \
        entry = next;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    free(ht->entries);                                                         \
    ht->entries = new_entries;                                                 \
    ht->capacity = new_capacity;                                               \
    return HT_SUCCESS;                                                         \
  }                                                                            \
                                                                               \
  ht_status name##_set(name##_t *ht, key_type key, value_type value) {         \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    if ((float)ht->size / ht->capacity > 0.75) {                               \
      if (name##_resize(ht) != HT_SUCCESS) {                                   \
        pthread_rwlock_unlock(&ht->rwlock);                                    \
        return HT_ERROR_MEMORY;                                                \
      }                                                                        \
    }                                                                          \
                                                                               \
    size_t index = ht->hash(key, ht->user_data) % ht->capacity;                \
    name##_entry_t *entry = ht->entries[index];                                \
                                                                               \
    while (entry) {                                                            \
      if (ht->compare(entry->key, key, ht->user_data)) {                       \
        if (ht->value_free)                                                    \
          ht->value_free((void *)(uintptr_t)entry->value, ht->user_data);      \
        entry->value = value;                                                  \
        pthread_rwlock_unlock(&ht->rwlock);                                    \
        return HT_SUCCESS;                                                     \
      }                                                                        \
      entry = entry->next;                                                     \
    }                                                                          \
                                                                               \
    entry = (name##_entry_t *)malloc(sizeof(name##_entry_t));                  \
    if (!entry) {                                                              \
      pthread_rwlock_unlock(&ht->rwlock);                                      \
      return HT_ERROR_MEMORY;                                                  \
    }                                                                          \
    entry->key = key;                                                          \
    entry->value = value;                                                      \
    entry->next = ht->entries[index];                                          \
    ht->entries[index] = entry;                                                \
    ht->size++;                                                                \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return HT_SUCCESS;                                                         \
  }                                                                            \
                                                                               \
  value_type name##_get(name##_t *ht, key_type key) {                          \
    pthread_rwlock_rdlock(&ht->rwlock);                                        \
    size_t index = ht->hash(key, ht->user_data) % ht->capacity;                \
    name##_entry_t *entry = ht->entries[index];                                \
                                                                               \
    while (entry) {                                                            \
      if (ht->compare(entry->key, key, ht->user_data)) {                       \
        value_type val = ht->value_copy                                        \
                             ? ht->value_copy(entry->value, ht->user_data)     \
                             : entry->value;                                   \
        pthread_rwlock_unlock(&ht->rwlock);                                    \
        return val;                                                            \
      }                                                                        \
      entry = entry->next;                                                     \
    }                                                                          \
                                                                               \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return (value_type)0;                                                      \
  }                                                                            \
                                                                               \
  int name##_delete(name##_t *ht, key_type key) {                              \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    size_t index = ht->hash(key, ht->user_data) % ht->capacity;                \
    name##_entry_t *entry = ht->entries[index];                                \
    name##_entry_t *prev = NULL;                                               \
                                                                               \
    while (entry) {                                                            \
      if (ht->compare(entry->key, key, ht->user_data)) {                       \
        if (prev) {                                                            \
          prev->next = entry->next;                                            \
        } else {                                                               \
          ht->entries[index] = entry->next;                                    \
        }                                                                      \
        if (ht->key_free)                                                      \
          ht->key_free((void *)(uintptr_t)entry->key, ht->user_data);          \
        if (ht->value_free)                                                    \
          ht->value_free((void *)(uintptr_t)entry->value, ht->user_data);      \
        free(entry);                                                           \
        ht->size--;                                                            \
        pthread_rwlock_unlock(&ht->rwlock);                                    \
        return 1;                                                              \
      }                                                                        \
      prev = entry;                                                            \
      entry = entry->next;                                                     \
    }                                                                          \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return 0;                                                                  \
  }                                                                            \
                                                                               \
  void name##_clear(name##_t *ht) {                                            \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    for (size_t i = 0; i < ht->capacity; i++) {                                \
      name##_entry_t *entry = ht->entries[i];                                  \
      while (entry) {                                                          \
        name##_entry_t *next = entry->next;                                    \
        if (ht->key_free)                                                      \
          ht->key_free((void *)(uintptr_t)entry->key, ht->user_data);          \
        if (ht->value_free)                                                    \
          ht->value_free((void *)(uintptr_t)entry->value, ht->user_data);      \
        free(entry);                                                           \
        entry = next;                                                          \
      }                                                                        \
      ht->entries[i] = NULL;                                                   \
    }                                                                          \
    ht->size = 0;                                                              \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
  }                                                                            \
                                                                               \
  ht_status name##_reserve(name##_t *ht, size_t capacity) {                    \
    pthread_rwlock_wrlock(&ht->rwlock);                                        \
    if (capacity <= ht->capacity) {                                            \
      pthread_rwlock_unlock(&ht->rwlock);                                      \
      return HT_SUCCESS;                                                       \
    }                                                                          \
    /* Resize logic duplicated/adapted from resize */                          \
    size_t new_capacity = capacity;                                            \
    /* Ensure power of 2 or just use as is? Let's just use as is for now, or   \
     * round up */                                                             \
    /* Simple implementation: just realloc entries array and rehash */         \
    name##_entry_t **new_entries =                                             \
        (name##_entry_t **)calloc(new_capacity, sizeof(name##_entry_t *));     \
    if (!new_entries) {                                                        \
      pthread_rwlock_unlock(&ht->rwlock);                                      \
      return HT_ERROR_MEMORY;                                                  \
    }                                                                          \
                                                                               \
    for (size_t i = 0; i < ht->capacity; i++) {                                \
      name##_entry_t *entry = ht->entries[i];                                  \
      while (entry) {                                                          \
        name##_entry_t *next = entry->next;                                    \
        size_t new_index = ht->hash(entry->key, ht->user_data) % new_capacity; \
        entry->next = new_entries[new_index];                                  \
        new_entries[new_index] = entry;                                        \
        entry = next;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    free(ht->entries);                                                         \
    ht->entries = new_entries;                                                 \
    ht->capacity = new_capacity;                                               \
    pthread_rwlock_unlock(&ht->rwlock);                                        \
    return HT_SUCCESS;                                                         \
  }                                                                            \
                                                                               \
  name##_iter_t name##_iter_create(name##_t *ht) {                             \
    name##_iter_t iter;                                                        \
    iter.ht = ht;                                                              \
    iter.index = 0;                                                            \
    iter.entry = NULL;                                                         \
    /* We need to lock the table during iteration? */                          \
    /* For a simple iterator, we might assume the user handles locking or we   \
     * lock for the whole duration? */                                         \
    /* Typically iterators are not thread-safe if the table is modified. */    \
    /* Let's just return the iterator. The user should be careful. */          \
    return iter;                                                               \
  }                                                                            \
                                                                               \
  int name##_iter_next(name##_iter_t *iter, key_type *key,                     \
                       value_type *value) {                                    \
    /* Find next entry */                                                      \
    if (iter->entry) {                                                         \
      iter->entry = iter->entry->next;                                         \
    }                                                                          \
    while (!iter->entry && iter->index < iter->ht->capacity) {                 \
      iter->entry = iter->ht->entries[iter->index++];                          \
    }                                                                          \
    if (iter->entry) {                                                         \
      if (key)                                                                 \
        *key = iter->entry->key;                                               \
      if (value)                                                               \
        *value = iter->entry->value;                                           \
      return 1;                                                                \
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