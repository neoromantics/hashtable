#define HT_IMPLEMENTATION
#include "../hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Custom Key Type: Employee ---
typedef struct {
  int id;
  char department[32];
} Employee;

// --- Custom Value Type: PersonalInfo ---
typedef struct {
  char name[64];
  char email[64];
  int age;
} PersonalInfo;

// --- Hash Function for Employee* ---
uint32_t hash_employee(const void *key, void *user_data) {
  (void)user_data;
  const Employee *emp = (const Employee *)key;
  // Combine ID and department for hash
  // Using a simple combination: hash(id) ^ hash(department)
  uint32_t h = murmur3_32((const uint8_t *)&emp->id, sizeof(emp->id), 0);
  h ^= murmur3_32((const uint8_t *)emp->department, strlen(emp->department), 0);
  return h;
}

// --- Compare Function for Employee* ---
int compare_employee(const void *key1, const void *key2, void *user_data) {
  (void)user_data;
  const Employee *e1 = (const Employee *)key1;
  const Employee *e2 = (const Employee *)key2;
  return (e1->id == e2->id) && (strcmp(e1->department, e2->department) == 0);
}

// --- Define Hash Table Type ---
// Mapping Employee* -> PersonalInfo*
HT_DEFINE(emp_ht, Employee *, PersonalInfo *)
HT_IMPLEMENT(emp_ht, Employee *, PersonalInfo *)

// --- Helper Functions ---
Employee *create_employee(int id, const char *dept) {
  Employee *e = malloc(sizeof(Employee));
  e->id = id;
  strncpy(e->department, dept, sizeof(e->department) - 1);
  e->department[sizeof(e->department) - 1] = '\0';
  return e;
}

PersonalInfo *create_info(const char *name, const char *email, int age) {
  PersonalInfo *p = malloc(sizeof(PersonalInfo));
  strncpy(p->name, name, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';
  strncpy(p->email, email, sizeof(p->email) - 1);
  p->email[sizeof(p->email) - 1] = '\0';
  p->age = age;
  return p;
}

int main() {
  printf("Creating Employee Hash Table...\n");

  // Create the hash table with custom functions
  // Use ht_free for standard free() behavior
  emp_ht_t *ht = emp_ht_create(hash_employee, compare_employee, ht_free,
                               ht_free, NULL, NULL);

  if (!ht) {
    fprintf(stderr, "Failed to create hash table.\n");
    return 1;
  }

  // 1. Insert Data
  printf("Inserting employees...\n");

  Employee *e1 = create_employee(101, "Engineering");
  PersonalInfo *p1 = create_info("Alice Smith", "alice@example.com", 30);
  emp_ht_set(ht, e1, p1);

  Employee *e2 = create_employee(102, "HR");
  PersonalInfo *p2 = create_info("Bob Jones", "bob@example.com", 45);
  emp_ht_set(ht, e2, p2);

  Employee *e3 = create_employee(
      101, "Sales"); // Same ID but different dept -> different key
  PersonalInfo *p3 = create_info("Charlie Brown", "charlie@example.com", 28);
  emp_ht_set(ht, e3, p3);

  // 2. Retrieve Data
  printf("\nRetrieving employees:\n");

  // Lookup Alice
  Employee lookup_e1 = {101, "Engineering"}; // Stack allocated key for lookup
  PersonalInfo *info = emp_ht_get(ht, &lookup_e1);
  if (info) {
    printf("Found: ID 101 (Eng) -> Name: %s, Email: %s, Age: %d\n", info->name,
           info->email, info->age);
  } else {
    printf("ID 101 (Eng) not found.\n");
  }

  // Lookup Bob
  Employee lookup_e2 = {102, "HR"};
  info = emp_ht_get(ht, &lookup_e2);
  if (info) {
    printf("Found: ID 102 (HR)  -> Name: %s, Email: %s, Age: %d\n", info->name,
           info->email, info->age);
  }

  // Lookup Non-existent
  Employee lookup_e4 = {999, "Marketing"};
  info = emp_ht_get(ht, &lookup_e4);
  if (!info) {
    printf("ID 999 (Marketing) not found (as expected).\n");
  }

  // 3. Iterate
  printf("\nIterating over all employees:\n");
  emp_ht_iter_t iter = emp_ht_iter_create(ht);
  Employee *key;
  PersonalInfo *val;
  while (emp_ht_iter_next(&iter, &key, &val)) {
    printf("- [%d, %s]: %s (%s)\n", key->id, key->department, val->name,
           val->email);
  }

  // 4. Delete
  printf("\nDeleting Bob...\n");
  if (emp_ht_delete(ht, &lookup_e2)) {
    printf("Bob deleted successfully.\n");
  } else {
    printf("Failed to delete Bob.\n");
  }

  // Verify deletion
  if (!emp_ht_get(ht, &lookup_e2)) {
    printf("Verified: Bob is gone.\n");
  }

  // 5. Cleanup
  printf("\nDestroying hash table...\n");
  emp_ht_destroy(ht); // This will free all remaining keys and values
  printf("Done.\n");

  return 0;
}
