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

#include "util.h"
#define UHASH_MINIMUM_CAPACITY 64
#define UHASH_LOAD_FACTOR 0.5

uhash_table *
uhash_init (size_t initial_capacity)
{
  initial_capacity = round_to_power_of_2 (initial_capacity);

  uhash_table *table = malloc (sizeof (uhash_table));
  if (!table)
    return NULL;

  table->entries = calloc (initial_capacity, sizeof (uhash_entry));
  if (!table->entries)
    {
      free (table);
      return NULL;
    }

  table->capacity = initial_capacity;
  table->count = 0;
  return table;
}

bool
uhash_resize (uhash_table *table)
{
  size_t new_capacity = table->capacity << 1;
  uhash_entry *new_entries = calloc (new_capacity, sizeof (uhash_entry));
  if (!new_entries)
    return false;

  size_t index_mask = new_capacity - 1;
  uhash_entry *old_entries = table->entries;

  for (size_t i = 0; i < table->capacity; i++)
    {
      if (old_entries[i].occupied)
        {
          size_t index = uhash_compute_hash (old_entries[i].key) & index_mask;
          while (new_entries[index].occupied)
            index = (index + 1) & index_mask;
          new_entries[index] = old_entries[i];
        }
    }

  free (old_entries);
  table->entries = new_entries;
  table->capacity = new_capacity;
  return true;
}

bool
uhash_has (uhash_table *table, uhash_key key)
{
  if (!table)
    return false;

  size_t index_mask = table->capacity - 1;
  size_t index = uhash_compute_hash (key) & index_mask;
  const size_t start_index = index;

  do
    {
      if (!table->entries[index].occupied)
        return false;
      if (uhash_keys_equal (table->entries[index].key, key))
        return true;
      index = (index + 1) & index_mask;
    }
  while (index != start_index);

  return false;
}

bool
uhash_add (uhash_table *table, uhash_key key)
{
  if (!table)
    return false;

  if (table->count * 2 >= table->capacity)
    if (!uhash_resize (table))
      return false;

  size_t index_mask = table->capacity - 1;
  size_t index = uhash_compute_hash (key) & index_mask;

  while (table->entries[index].occupied)
    {
      if (uhash_keys_equal (table->entries[index].key, key))
        return false;
      index = (index + 1) & index_mask;
    }

  table->entries[index].key = key;
  table->entries[index].occupied = true;
  table->count++;
  return true;
}

void
uhash_nuke (uhash_table *table)
{
  if (table)
    {
      free (table->entries);
      free (table);
    }
}
