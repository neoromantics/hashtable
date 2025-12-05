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

## Building and Testing

The project includes a `Makefile` for building tests and examples.

```bash
# Build everything
make

# Run tests
make test

# Run benchmark
make bench

# Run custom types example
make example_custom_types && ./example_custom_types
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

## Built-in Hash Functions

The library provides several built-in hash and comparison functions for convenience:

*   `ht_hash_string` / `ht_compare_string`: For null-terminated C strings.
*   `ht_hash_int` / `ht_compare_int`: For integer keys (cast to `void*`).
*   `ht_hash_ptr` / `ht_compare_ptr`: For pointer identity (address) hashing.

**Default Behavior**: If you pass `NULL` for the hash and compare functions in `create`, the library defaults to **pointer identity hashing** (`ht_hash_ptr`), which is useful for generic pointer keys.

## Advanced Usage

### Custom Types

To use custom structures as keys, you must provide hash and comparison functions. It is recommended to use pointers to structures as keys (`Struct*`) rather than the structures themselves (`Struct`) to avoid copying overhead.

See `examples/example_custom_types.c` for a complete, compilable example of using custom structs for both keys and values, including memory management.

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
| `name##_create` | Allocates and initializes a new hash table. Defaults to identity hash if funcs are NULL. |
| `name##_destroy` | Frees the hash table and all its entries. |
| `name##_set` | Inserts or updates a key-value pair. Returns `HT_SUCCESS` or `HT_ERROR_MEMORY`. |
| `name##_get` | Retrieves the value associated with a key. Returns zero-value/NULL if not found. |
| `name##_delete` | Removes a key-value pair. Returns `1` if deleted, `0` if not found. |
| `name##_clear` | Removes all entries but retains allocated capacity. |
| `name##_reserve` | Pre-allocates memory to accommodate at least `capacity` items. |
| `name##_iter_create` | Creates a new iterator for the table. |
| `name##_iter_next` | Advances the iterator. Returns `1` if valid, `0` at end. |
