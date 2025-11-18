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

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#else
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <stdatomic.h>
#endif
#else
#include <dirent.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(_OPENMP) && !defined(_MSC_VER)
#include <omp.h>
#endif

#include "platform.h"
#include "util.h"

#define MAX_TASK_DEPTH 64

#ifdef _MSC_VER
static volatile long long global_directory_count, global_file_count,
    global_byte_count;
#else
static atomic_uint_fast64_t global_directory_count;
static atomic_uint_fast64_t global_file_count, global_byte_count;
#endif

// Platform-specific atomic interface
#ifdef _MSC_VER

void
platform_add_directories (size_t n)
{
  _InterlockedExchangeAdd64 (&global_directory_count, (long long)(n));
}

void
platform_add_files (uint64_t n)
{
  _InterlockedExchangeAdd64 (&global_file_count, (long long)(n));
}

void
platform_add_bytes (uint64_t n)
{
  _InterlockedExchangeAdd64 (&global_byte_count, (long long)(n));
}

size_t
platform_load_directories (void)
{
  return (size_t)_InterlockedOr64 (&global_directory_count, 0);
}

uint64_t
platform_load_files (void)
{
  return (uint64_t)_InterlockedOr64 (&global_file_count, 0);
}

uint64_t
platform_load_bytes (void)
{
  return (uint64_t)_InterlockedOr64 (&global_byte_count, 0);
}

#else

void
platform_add_directories (size_t n)
{
  atomic_fetch_add_explicit (&global_directory_count, (uint64_t)n,
                             memory_order_relaxed);
}

void
platform_add_files (uint64_t n)
{
  atomic_fetch_add_explicit (&global_file_count, n, memory_order_relaxed);
}

void
platform_add_bytes (uint64_t n)
{
  atomic_fetch_add_explicit (&global_byte_count, n, memory_order_relaxed);
}

size_t
platform_load_directories (void)
{
  return (size_t)atomic_load (&global_directory_count);
}

uint64_t
platform_load_files (void)
{
  return (uint64_t)atomic_load (&global_file_count);
}

uint64_t
platform_load_bytes (void)
{
  return (uint64_t)atomic_load (&global_byte_count);
}

#endif

void
platform_path_join (char *destination, size_t size, const char *base,
                    const char *name)
{
#ifdef _WIN32
  snprintf (destination, size, "%s\\%s", base, name);
#else
  size_t base_length = strlen (base);
  snprintf (destination, size,
            (base_length > 0 && base[base_length - 1] == '/') ? "%s%s"
                                                              : "%s/%s",
            base, name);
#endif
}

// Windows disk usage calculation
#ifdef _WIN32
static uint64_t global_cluster_size = 0;

static uint64_t
calculate_disk_usage_windows (const char *path, uint64_t size)
{
  if (!global_cluster_size)
    {
      char volume[PATH_MAX];
      DWORD sectors_per_cluster = 0, bytes_per_sector = 0, free_clusters = 0,
            total_clusters = 0;
      if (GetVolumePathNameA (path, volume, sizeof (volume))
          && GetDiskFreeSpaceA (volume, &sectors_per_cluster,
                                &bytes_per_sector, &free_clusters,
                                &total_clusters))
        global_cluster_size = (uint64_t)sectors_per_cluster * bytes_per_sector;
      else
        global_cluster_size = 4096;
    }

  return ((size + global_cluster_size - 1) / global_cluster_size)
         * global_cluster_size;
}
#endif

//  Platform-agnostic size calculation
uint64_t
platform_calculate_size (const char *path, uint64_t apparent_size,
                         bool use_disk_usage)
{
  if (!use_disk_usage)
    return apparent_size;

#ifdef _WIN32
  return calculate_disk_usage_windows (path, apparent_size);
#else
  /* On Unix, caller already provides st_blocks*512 when disk usage is
   * requested. */
  return apparent_size;
#endif
}

// Windows file processing
#ifdef _WIN32
static bool
process_hardlink_windows (const char *full_path, uhash_table *hash_table,
                          HANDLE file_handle)
{
  BY_HANDLE_FILE_INFORMATION info;
  if (!GetFileInformationByHandle (file_handle, &info)
      || info.nNumberOfLinks <= 1)
    return false;

  uhash_key key;
  key.device = info.dwVolumeSerialNumber;
  key.inode = ((uint64_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;

  bool skip;
#ifdef _OPENMP
#pragma omp critical
#endif
  {
    skip = uhash_has (hash_table, key) || !uhash_add (hash_table, key);
  }
  return skip;
}

static bool
process_file_windows (const char *path, const char *full_path,
                      WIN32_FIND_DATAA *find_data, options opts,
                      uhash_table *hash_table, uint64_t *local_files,
                      uint64_t *local_bytes)
{
  uint64_t file_size
      = ((uint64_t)find_data->nFileSizeHigh << 32) | find_data->nFileSizeLow;
  uint64_t size
      = platform_calculate_size (full_path, file_size, opts.disk_usage);
  bool skip = false;

  if (opts.disk_usage && hash_table)
    {
      HANDLE file_handle
          = CreateFileA (full_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, 0, NULL);
      if (file_handle != INVALID_HANDLE_VALUE)
        {
          skip = process_hardlink_windows (full_path, hash_table, file_handle);
          CloseHandle (file_handle);
        }
    }

  if (!skip)
    {
      (*local_files)++;
      (*local_bytes) += size;
      if (opts.verbose)
        {
          char human_size[16];

          human_readable (human_size, size);
          printf ("%-10s %s\n", human_size, full_path);
        }
    }

  return true;
}
#endif

// Unix file processing
#ifndef _WIN32
static bool
process_hardlink_unix (struct stat *stat_buffer, uhash_table *hash_table)
{
  if (!S_ISREG (stat_buffer->st_mode) || stat_buffer->st_nlink <= 1)
    return false;

  uhash_key key;
  key.device = stat_buffer->st_dev;
  key.inode = stat_buffer->st_ino;

  bool skip;
#ifdef _OPENMP
#pragma omp critical
#endif
  {
    skip = uhash_has (hash_table, key) || !uhash_add (hash_table, key);
  }
  return skip;
}

static bool
process_file_unix (const char *full_path, struct stat *stat_buffer,
                   options opts, uhash_table *hash_table,
                   uint64_t *local_files, uint64_t *local_bytes)
{
  uint64_t size = opts.disk_usage ? (uint64_t)stat_buffer->st_blocks * 512
                                  : (uint64_t)stat_buffer->st_size;
  bool skip = false;

  if (opts.disk_usage && hash_table)
    skip = process_hardlink_unix (stat_buffer, hash_table);

  if (!skip)
    {
      (*local_files)++;
      (*local_bytes) += size;
      if (opts.verbose)
        {
          char human_size[16];

          human_readable (human_size, size);
          printf ("%-10s %s\n", human_size, full_path);
        }
    }

  return true;
}
#endif

// Directory scanning engine
void
platform_scan_directory (const char *path, const excludes_list *excludes,
                         options opts, uhash_table *hash_table, int depth)
{
  if (!path)
    return;

  uint64_t local_directories = 1;
  uint64_t local_files = 0;
  uint64_t local_bytes = 0;

#ifdef _WIN32
  WIN32_FIND_DATAA find_data;
  char search_pattern[PATH_MAX];
  snprintf (search_pattern, sizeof (search_pattern), "%s\\*", path);

  HANDLE find_handle = FindFirstFileA (search_pattern, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE)
    return;

  do
    {
      const char *name = find_data.cFileName;
      if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2])))
        continue;

      char full_path[PATH_MAX];
      platform_path_join (full_path, sizeof (full_path), path, name);

      if (excludes_contains (excludes, name)
          || excludes_contains (excludes, full_path))
        continue;

      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
#if defined(_OPENMP) && !defined(_MSC_VER)
          if (depth < MAX_TASK_DEPTH)
            {
#pragma omp task firstprivate(full_path, depth, opts)                         \
    shared(excludes, hash_table)
              platform_scan_directory (full_path, excludes, opts, hash_table,
                                       depth + 1);
            }
          else
#endif
            platform_scan_directory (full_path, excludes, opts, hash_table,
                                     depth + 1);
        }
      else
        {
          process_file_windows (path, full_path, &find_data, opts, hash_table,
                                &local_files, &local_bytes);
        }
    }
  while (FindNextFileA (find_handle, &find_data));
  FindClose (find_handle);

#else
  DIR *directory = opendir (path);
  if (!directory)
    return;

  struct dirent *entry;
  while ((entry = readdir (directory)))
    {
      const char *name = entry->d_name;
      if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2])))
        continue;

      char full_path[PATH_MAX];
      platform_path_join (full_path, sizeof (full_path), path, name);
      if (excludes_contains (excludes, name)
          || excludes_contains (excludes, full_path))
        continue;

      struct stat stat_buffer;
      if (lstat (full_path, &stat_buffer))
        continue;

      if (S_ISDIR (stat_buffer.st_mode))
        {
#if defined(_OPENMP) && !defined(_MSC_VER)
          if (depth < MAX_TASK_DEPTH)
            {
#pragma omp task firstprivate(full_path, depth, opts)                         \
    shared(excludes, hash_table)
              platform_scan_directory (full_path, excludes, opts, hash_table,
                                       depth + 1);
            }
          else
#endif
            platform_scan_directory (full_path, excludes, opts, hash_table,
                                     depth + 1);
        }
      else if (S_ISREG (stat_buffer.st_mode) || S_ISLNK (stat_buffer.st_mode))
        {
          process_file_unix (full_path, &stat_buffer, opts, hash_table,
                             &local_files, &local_bytes);
        }
    }
  closedir (directory);
#endif

  platform_add_directories (local_directories);
  platform_add_files (local_files);
  platform_add_bytes (local_bytes);
}
