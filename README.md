# Generic C Hash Table

## Overview

`hashtable.h` is a header-only, generic, thread-safe hash table implementation in C. It utilizes C preprocessor macros to generate type-safe definitions for keys and values, avoiding the need for `void*` casting in the primary API and providing better compile-time safety. It employs MurmurHash3 for efficient hashing and `pthread_rwlock` for concurrent access.

## Integration

To use this library, include `hashtable.h` in your source files. In exactly one source file, define `HT_IMPLEMENTATION` before including the header to generate the implementation code.

```c
// main.c (or any single implementation file)
#define HT_IMPLEMENTATION
#include "hashtable.h"
```

## Design Philosophy

This library uses C preprocessor macros to define type-specific hash table structures and functions. This approach allows for:

*   **Type Safety**: Keys and values are typed in the API, reducing runtime errors associated with `void*`.
*   **Performance**: Inlining of comparison and hash functions can potentially allow for compiler optimizations.
*   **Simplicity**: No external dependencies beyond the standard C library and pthreads.

## Quick Start

The following example demonstrates how to define and use a hash table mapping string keys (`char*`) to integer values (`int`).

```c
#define HT_IMPLEMENTATION
#include "hashtable.h"
#include <stdio.h>

// Define the hash table type 'str_map' mapping char* -> int
HT_DEFINE(str_map, char*, int)
HT_IMPLEMENT(str_map, char*, int)

int main(void) {
    // Create the hash table using default string hash/compare functions
    // Arguments: hash_func, compare_func, key_free_func, value_free_func, value_copy_func, user_data
    str_map_t *map = str_map_create(ht_hash_string, ht_compare_string, NULL, NULL, NULL, NULL);

    if (!map) {
        fprintf(stderr, "Failed to create hash table\n");
        return 1;
    }

    // Insert
    str_map_set(map, "key1", 100);
    str_map_set(map, "key2", 200);

    // Retrieve
    int val = str_map_get(map, "key1");
    printf("key1: %d\n", val);

    // Iterate
    str_map_iter_t iter = str_map_iter_create(map);
    char *k;
    int v;
    while (str_map_iter_next(&iter, &k, &v)) {
        printf("%s -> %d\n", k, v);
    }

    // Cleanup
    str_map_destroy(map);
    return 0;
}
```

## Advanced Usage

### Custom Types

To use custom structures as keys, you must provide hash and comparison functions. It is recommended to use pointers to structures as keys (`Struct*`) rather than the structures themselves (`Struct`) to avoid copying overhead and to ensure compatibility with the internal `void*` storage.

The following is a complete, compilable example of using a custom `Point` struct as a key.

```c
#define HT_IMPLEMENTATION
#include "hashtable.h"
#include <stdio.h>

typedef struct {
    int x, y;
} Point;

// Hash function for Point*
uint32_t hash_point(const void *key, void *user_data) {
    const Point *p = (const Point *)key;
    // Simple hash combination
    return (uint32_t)((p->x * 31) ^ p->y);
}

// Compare function for Point*
int compare_point(const void *key1, const void *key2, void *user_data) {
    const Point *p1 = (const Point *)key1;
    const Point *p2 = (const Point *)key2;
    return (p1->x == p2->x) && (p1->y == p2->y);
}

// Define table mapping Point* -> int
HT_DEFINE(point_map, Point*, int)
HT_IMPLEMENT(point_map, Point*, int)

int main(void) {
    // Create the hash table
    point_map_t *map = point_map_create(hash_point, compare_point, NULL, NULL, NULL, NULL);

    if (!map) {
        fprintf(stderr, "Failed to create hash table\n");
        return 1;
    }

    // Create some points
    Point p1 = {1, 2};
    Point p2 = {3, 4};
    Point p3 = {5, 6};

    // Insert using pointers to the points
    point_map_set(map, &p1, 100);
    point_map_set(map, &p2, 200);

    // Retrieve
    int val = point_map_get(map, &p1);
    printf("Point(%d, %d) -> %d\n", p1.x, p1.y, val);

    // Update
    point_map_set(map, &p1, 150);
    printf("Point(%d, %d) updated -> %d\n", p1.x, p1.y, point_map_get(map, &p1));

    // Check for non-existent key
    if (point_map_get(map, &p3) == 0) {
        printf("Point(%d, %d) not found\n", p3.x, p3.y);
    }

    // Cleanup
    point_map_destroy(map);
    return 0;
}
```

### Complete Example: Custom Key and Custom Value

This example demonstrates using a custom `Point` struct as a key and a `Person` struct as a value. It also shows how to handle memory management for dynamically allocated keys and values.

```c
#define HT_IMPLEMENTATION
#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Key Definition ---
typedef struct {
    int x, y;
} Point;

uint32_t hash_point(const void *key, void *user_data) {
    const Point *p = (const Point *)key;
    return (uint32_t)((p->x * 31) ^ p->y);
}

int compare_point(const void *key1, const void *key2, void *user_data) {
    const Point *p1 = (const Point *)key1;
    const Point *p2 = (const Point *)key2;
    return (p1->x == p2->x) && (p1->y == p2->y);
}

// --- Value Definition ---
typedef struct {
    char name[32];
    int age;
} Person;

// --- Memory Management Callbacks ---
void free_key(void *key, void *user_data) {
    free(key);
}

void free_value(void *value, void *user_data) {
    free(value);
}

// --- Table Definition ---
// Mapping Point* -> Person*
HT_DEFINE(person_map, Point*, Person*)
HT_IMPLEMENT(person_map, Point*, Person*)

int main(void) {
    // Create table with free functions
    person_map_t *map = person_map_create(hash_point, compare_point, free_key, free_value, NULL, NULL);

    // Helper to create a point
    Point *p1 = malloc(sizeof(Point));
    p1->x = 10; p1->y = 20;

    // Helper to create a person
    Person *person1 = malloc(sizeof(Person));
    strcpy(person1->name, "Alice");
    person1->age = 30;

    // Insert
    // The table now owns the memory for p1 and person1
    person_map_set(map, p1, person1);

    // Retrieve
    // We need a temporary key for lookup (stack allocated is fine)
    Point lookup_key = {10, 20};
    Person *retrieved = person_map_get(map, &lookup_key);

    if (retrieved) {
        printf("Found: %s, Age: %d\n", retrieved->name, retrieved->age);
    }

    // Cleanup
    // This will automatically call free_key(p1) and free_value(person1)
    person_map_destroy(map);
    return 0;
}
```

### Memory Management

The `create` function accepts optional callbacks for freeing keys and values. These are invoked when an item is deleted or when the table is destroyed.

*   `key_free`: Called to free the key.
*   `value_free`: Called to free the value.
*   `value_copy`: Called when retrieving a value (optional, supports deep copy on retrieval).

```c
void my_key_free(void *key, void *user_data) {
    free(key);
}

// Usage:
// str_map_create(..., my_key_free, ...);
```

### Concurrency

The implementation uses `pthread_rwlock_t` to ensure thread safety.

*   **Write Operations** (`set`, `delete`, `clear`, `reserve`): Acquire a write lock.
*   **Read Operations** (`get`): Acquire a read lock.
*   **Iteration**: Iterators are **not** thread-safe if the table is modified during iteration. External synchronization is required if you intend to iterate while other threads may be modifying the table.

## API Reference

The macros `HT_DEFINE(name, key_type, value_type)` and `HT_IMPLEMENT(name, key_type, value_type)` generate the following API, where `name` is the prefix provided.

| Function | Description |
| :--- | :--- |
| `name##_create` | Allocates and initializes a new hash table. |
| `name##_destroy` | Frees the hash table and all its entries. |
| `name##_set` | Inserts or updates a key-value pair. Returns `HT_SUCCESS` or `HT_ERROR_MEMORY`. |
| `name##_get` | Retrieves the value associated with a key. Returns zero-value/NULL if not found. |
| `name##_delete` | Removes a key-value pair. Returns `1` if deleted, `0` if not found. |
| `name##_clear` | Removes all entries but retains allocated capacity. |
| `name##_reserve` | Pre-allocates memory to accommodate at least `capacity` items. |
| `name##_iter_create` | Creates a new iterator for the table. |
| `name##_iter_next` | Advances the iterator. Returns `1` if valid, `0` at end. |
