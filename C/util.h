/* Copyright (C) 2023, 2024, 2025 Ali Almalki <gnualmalki@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UDU_UTIL_H
#define UDU_UTIL_H

#include <stdbool.h>
#include <stdint.h>
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

typedef struct
{
  dev_t dev;
  ino_t ino;
} UhashKey;

typedef struct
{
  UhashKey key;
  bool occupied;
} UhashEntry;

typedef struct
{
  UhashEntry *entries;
  size_t capacity;
  size_t count;
} Uhash;

#define UHASH_MIN_CAP 64
#define UHASH_LOAD 0.5

static inline uint64_t
uhash_hash (UhashKey k)
{
  return XXH3_64bits (&k.dev, sizeof (k.dev))
         ^ XXH3_64bits (&k.ino, sizeof (k.ino));
}

static inline bool
uhash_equal (UhashKey a, UhashKey b)
{
  return (a.dev == b.dev) && (a.ino == b.ino);
}

Uhash *
uhash_init (size_t cap)
{
  if (cap < UHASH_MIN_CAP)
    cap = UHASH_MIN_CAP;

  // round to next power of 2
  cap--;
  cap |= cap >> 1;
  cap |= cap >> 2;
  cap |= cap >> 4;
  cap |= cap >> 8;
  cap |= cap >> 16;
#if SIZE_MAX > UINT32_MAX // 64bit
  cap |= cap >> 32;
#endif
  cap++;

  Uhash *ht = malloc (sizeof (Uhash));
  if (!ht)
    return NULL;

  ht->entries = calloc (cap, sizeof (UhashEntry));
  if (!ht->entries)
    {
      free (ht);
      return NULL;
    }

  ht->capacity = cap;
  ht->count = 0;
  return ht;
}

static bool
uhash_resize (Uhash *ht)
{
  size_t new_cap = ht->capacity << 1;
  UhashEntry *new_ent = calloc (new_cap, sizeof (UhashEntry));
  if (!new_ent)
    return false;

  size_t mask = new_cap - 1;
  UhashEntry *old = ht->entries;

  for (size_t i = 0; i < ht->capacity; i++)
    {
      if (old[i].occupied)
        {
          size_t idx = uhash_hash (old[i].key) & mask;
          while (new_ent[idx].occupied)
            idx = (idx + 1) & mask;
          new_ent[idx] = old[i];
        }
    }

  free (old);
  ht->entries = new_ent;
  ht->capacity = new_cap;
  return true;
}

bool
uhash_has (Uhash *ht, UhashKey k)
{
  if (!ht)
    return false;

  size_t mask = ht->capacity - 1;
  size_t idx = uhash_hash (k) & mask;
  size_t start = idx;

  do
    {
      if (!ht->entries[idx].occupied)
        return false;
      if (uhash_equal (ht->entries[idx].key, k))
        return true;
      idx = (idx + 1) & mask;
    }
  while (idx != start);

  return false;
}

bool
uhash_add (Uhash *ht, UhashKey k)
{
  if (!ht)
    return false;

  if (ht->count * 2 >= ht->capacity)
    if (!uhash_resize (ht))
      return false;

  size_t mask = ht->capacity - 1;
  size_t idx = uhash_hash (k) & mask;

  while (ht->entries[idx].occupied)
    {
      if (uhash_equal (ht->entries[idx].key, k))
        return false;
      idx = (idx + 1) & mask;
    }

  ht->entries[idx].key = k;
  ht->entries[idx].occupied = true;
  ht->count++;
  return true;
}

void
uhash_nuke (Uhash *ht)
{
  if (ht)
    {
      free (ht->entries);
      free (ht);
    }
}

//////// platform helpers
#ifdef _WIN32

static inline bool
uhash_key_from_handle (HANDLE h, UhashKey *k)
{
  BY_HANDLE_FILE_INFORMATION info;
  if (!GetFileInformationByHandle (h, &info))
    return false;
  k->dev = info.dwVolumeSerialNumber;
  k->ino = ((uint64_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;
  return true;
}

static inline bool
uhash_key_from_path (const char *path, UhashKey *k)
{
  HANDLE h = CreateFileA (
      path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (h == INVALID_HANDLE_VALUE)
    return false;

  bool ok = uhash_key_from_handle (h, k);
  CloseHandle (h);
  return ok;
}

#else

static inline bool
uhash_key_from_stat (const struct stat *st, UhashKey *k)
{
  k->dev = st->st_dev;
  k->ino = st->st_ino;
  return true;
}

static inline bool
uhash_key_from_path (const char *path, UhashKey *k)
{
  struct stat st;
  return stat (path, &st) == 0 && uhash_key_from_stat (&st, k);
}

#endif

#endif // UDU_UTIL_H
