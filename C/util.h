#ifndef UDU_UTIL_H
#define UDU_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#ifdef _WIN32
#include <windows.h>
typedef uint32_t dev_t;
typedef uint64_t ino_t;
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "types.h"

typedef struct
{
  dev_t device;
  ino_t inode;
} uhash_key;

typedef struct
{
  uhash_key key;
  bool occupied;
} uhash_entry;

typedef struct
{
  uhash_entry *entries;
  size_t capacity;
  size_t count;
} uhash_table;

static inline uint64_t
uhash_compute_hash (uhash_key key)
{
  return XXH3_64bits (&key.device, sizeof (key.device))
         ^ XXH3_64bits (&key.inode, sizeof (key.inode));
}

static inline bool
uhash_keys_equal (uhash_key first, uhash_key second)
{
  return (first.device == second.device) && (first.inode == second.inode);
}

static inline size_t
round_to_power_of_2 (size_t n)
{
  if (n < 64)
    n = 64;

  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
#if SIZE_MAX > UINT32_MAX
  n |= n >> 32;
#endif
  n++;
  return n;
}

static void
human_readable (char *buffer, uint64_t value)
{
  static const char units[] = "BKMGTPEZY";
  size_t unit_index = 0;
  const size_t units_length = sizeof (units) - 1;
  double number = (double)value;

  while (number >= 1024.0 && unit_index < units_length)
    {
      number /= 1024.0;
      unit_index++;
    }

  if (unit_index)
    snprintf (buffer, 16, "%.1f%c", number, units[unit_index]);
  else
    snprintf (buffer, 16, "%llu", (unsigned long long)value);
}

static void
fmt_kcomma (char *buffer, uint64_t value)
{
  if (!buffer)
    return;

  if (value == 0)
    {
      buffer[0] = '0';
      buffer[1] = '\0';
      return;
    }

  char tmp[32];
  int n = 0;

  uint64_t v = value;
  do
    {
      tmp[n++] = (char)('0' + (v % 10));
      v /= 10;
    }
  while (v);

  if (n < 6) // no formating
    {
      for (int i = 0; i < n; i++)
        buffer[i] = tmp[n - 1 - i];

      buffer[n] = '\0';
      return;
    }

  int rem = n % 3;
  if (rem == 0)
    rem = 3;

  int o = 0;
  for (int i = n - 1; i >= 0; i--)
    {
      if (rem == 0)
        {
          buffer[o++] = ',';
          rem = 3;
        }
      buffer[o++] = tmp[i];
      rem--;
    }

  buffer[o] = '\0';
}

// EXCLUDES

#define EXCLUDES_INITIAL_CAPACITY 4

static void
excludes_free (excludes_list *excludes)
{
  if (!excludes)
    return;

  for (size_t i = 0; i < excludes->length; i++)
    free (excludes->items[i]);
  free (excludes->items);
  excludes->items = NULL;
  excludes->length = 0;
  excludes->capacity = 0;
}

static bool
excludes_add (excludes_list *excludes, const char *value)
{
  if (!excludes || !value)
    return false;

  if (excludes->length >= excludes->capacity)
    {
      size_t new_capacity = excludes->capacity ? excludes->capacity << 1
                                               : EXCLUDES_INITIAL_CAPACITY;
      char **new_items
          = (char **)realloc (excludes->items, new_capacity * sizeof (char *));
      if (!new_items)
        return false;
      excludes->items = new_items;
      excludes->capacity = new_capacity;
    }

  excludes->items[excludes->length] = strdup (value);
  if (!excludes->items[excludes->length])
    return false;
  excludes->length++;
  return true;
}

static bool
excludes_contains (const excludes_list *excludes, const char *value)
{
  if (!excludes || !value)
    return false;

  for (size_t i = 0; i < excludes->length; i++)
    if (strcmp (excludes->items[i], value) == 0)
      return true;

  return false;
}

uhash_table *uhash_init (size_t initial_capacity);
bool uhash_resize (uhash_table *table);
bool uhash_has (uhash_table *table, uhash_key key);
bool uhash_add (uhash_table *table, uhash_key key);
void uhash_nuke (uhash_table *table);

#endif
