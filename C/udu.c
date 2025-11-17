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

#include <stddef.h>
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS // shuting up on win
#else
// enable gnu extensions
#define _GNU_SOURCE
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_OPENMP) && !defined(_MSC_VER)
#include <omp.h>
#endif

#if defined(_WIN32)
#include <windows.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#ifdef _MSC_VER
#define strdup _strdup
#include <intrin.h>

#else

#include <stdatomic.h>
#endif

#else

#include <dirent.h>
#include <stdatomic.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#endif

#include "util.h"

#define MAX_TASK_DEPTH 64
#define EX_INIT_CAP 4

#ifdef _MSC_VER
// Win atomic interface

static volatile long long g_dirs, g_files, g_bytes;
#define add_dirs(n) _InterlockedExchangeAdd64 (&g_dirs, (long long)(n))
#define add_files(n) _InterlockedExchangeAdd64 (&g_files, (long long)(n))
#define add_bytes(n) _InterlockedExchangeAdd64 (&g_bytes, (long long)(n))
#define load_dirs() ((size_t)_InterlockedOr64 (&g_dirs, 0))
#define load_files() ((uint64_t)_InterlockedOr64 (&g_files, 0))
#define load_bytes() ((uint64_t)_InterlockedOr64 (&g_bytes, 0))

#else
// glibc interface

static atomic_ullong g_dirs;
static atomic_uint_fast64_t g_files, g_bytes;
#define add_dirs(n)                                                           \
  atomic_fetch_add_explicit (&g_dirs, n, memory_order_relaxed)
#define add_files(n)                                                          \
  atomic_fetch_add_explicit (&g_files, n, memory_order_relaxed)
#define add_bytes(n)                                                          \
  atomic_fetch_add_explicit (&g_bytes, n, memory_order_relaxed)

#define load_dirs() atomic_load (&g_dirs)
#define load_files() atomic_load (&g_files)
#define load_bytes() atomic_load (&g_bytes)
#endif

typedef struct
{
  char **items;
  size_t len, cap;
} Excludes;

typedef struct
{
  bool verbose;
  bool disk_usage; // default
  bool quiet;      // default

} Opts;

static const char *USAGE
    = "Usage: udu <path> [-ex=<name|path>] [-sb|--size-apparent] "
      "[-v|--verbose] [-q|--quiet] [-h|--help] [--version]\n\n"
      "OPTIONS:\n"
      "  -ex=<name|path>        Exclude file or directory\n"
      "  -sb, --size-apparent   Enable size apparent mode\n"
      "  -v, --verbose          Enable verbose output\n"
      "  -q, --quiet            Enable quiet output (default)\n"
      "  -h, --help             Print this help message\n"
      "  --version              Print program version\n\n"
      "Report bugs to: <https://github.com/makestatic/udu/issues>\nUDU home "
      "page: <https://github.com/makestatic/udu>\n";

static const char *LICENSE
    = "Copyright (C) 2023, 2024, 2025  Ali Almalki.\nLicense GPLv3+: GNU GPL "
      "version 3 or later <https://gnu.org/licenses/gpl.html>\nThis is free "
      "software: you are free to change and redistribute it.\nThere is NO "
      "WARRANTY, to the extent permitted by law.\n";

static void
human_size (char *buf, uint64_t v)
{
  static const char u[] = "BKMGTPEZY";
  size_t i = 0;
  size_t len = strlen (u);
  double n = (double)v;

  while (n >= 1024.0 && i < len)
    {
      n /= 1024;
      i++;
    }

  if (i)
    snprintf (buf, 16, "%.1f%c", n, u[i]);
  else
    snprintf (buf, 16, "%llu", (unsigned long long)v);
}

static void
fmt_comma (char *buf, uint64_t v)
{
  if (!v)
    {
      buf[0] = '0';
      buf[1] = '\0';
      return;
    }

  char t[32];
  int p = 0;
  while (v)
    {
      t[p++] = (char)('0' + v % 10);
      v /= 10;
    }

  int r = 0;
  for (int i = p - 1; i >= 0; i--)
    {
      if (r && (p - i - 1) % 3 == 0)
        buf[r++] = ',';

      buf[r++] = t[i];
    }

  buf[r] = 0;
}

static void
ex_free (Excludes *ex)
{
  if (!ex)
    return;

  for (size_t i = 0; i < ex->len; i++)
    free (ex->items[i]);
  free (ex->items);
}

static bool
ex_add (Excludes *ex, const char *v)
{
  if (ex->len >= ex->cap)
    {
      size_t nc = ex->cap ? ex->cap << 1 : EX_INIT_CAP;
      char **ni = realloc (ex->items, nc * sizeof (char *));
      if (!ni)
        return false;
      ex->items = ni;
      ex->cap = nc;
    }

  ex->items[ex->len] = strdup (v);
  return ex->items[ex->len++] != NULL;
}

static bool
ex_has (const Excludes *ex, const char *v)
{
  for (size_t i = 0; i < ex->len; i++)
    if (strcmp (ex->items[i], v) == 0)
      return true;

  return false;
}

static void
path_join (char *dst, size_t sz, const char *b, const char *n)
{
#ifdef _WIN32
  snprintf (dst, sz, "%s\\%s", b, n);
#else
  size_t l = strlen (b);
  snprintf (dst, sz, (b[l - 1] == '/') ? "%s%s" : "%s/%s", b, n);
#endif
}

#ifdef _WIN32
static uint64_t g_cluster = 0;

static uint64_t
get_disk_usage (const char *path, uint64_t sz)
{
  if (!g_cluster)
    {
      char vol[PATH_MAX];
      DWORD spc, bps, fc, tc;
      if (GetVolumePathNameA (path, vol, sizeof (vol))
          && GetDiskFreeSpaceA (vol, &spc, &bps, &fc, &tc))
        g_cluster = (uint64_t)spc * bps;
      else
        g_cluster = 4096;
    }

  return ((sz + g_cluster - 1) / g_cluster) * g_cluster;
}
#endif

static void
scan (const char *p, const Excludes *ex, Opts o, Uhash *ht, int d)
{
  if (!p)
    return;

  uint64_t ld = 1, lf = 0, lb = 0;

#ifdef _WIN32
  WIN32_FIND_DATAA fd;
  char s[PATH_MAX];
  snprintf (s, sizeof (s), "%s\\*", p);

  HANDLE h = FindFirstFileA (s, &fd);

  if (h == INVALID_HANDLE_VALUE)
    return;

  do
    {
      const char *n = fd.cFileName;
      if (n[0] == '.' && (!n[1] || (n[1] == '.' && !n[2])))
        continue;

      char fp[PATH_MAX];
      path_join (fp, sizeof (fp), p, n);

      if (ex_has (ex, n) || ex_has (ex, fp))
        continue;

      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
#if defined(_OPENMP) && !defined(_MSC_VER)
          if (d < MAX_TASK_DEPTH)
            {
#pragma omp task firstprivate(fp, d, o) shared(ex, ht)
              scan (fp, ex, o, ht, d + 1);
            }
          else
#endif
            scan (fp, ex, o, ht, d + 1);
        }
      else
        {
          uint64_t fsz = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
          uint64_t sz = o.disk_usage ? get_disk_usage (fp, fsz) : fsz;
          bool skip = false;

          if (o.disk_usage && ht)
            {
              HANDLE fh
                  = CreateFileA (fp, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);
              if (fh != INVALID_HANDLE_VALUE)
                {
                  BY_HANDLE_FILE_INFORMATION inf;

                  if (GetFileInformationByHandle (fh, &inf)
                      && inf.nNumberOfLinks > 1)
                    {
                      UhashKey k;

                      if (uhash_key_from_handle (fh, &k))
                        {
#ifdef _OPENMP
#pragma omp critical
#endif
                          skip = uhash_has (ht, k) || !uhash_add (ht, k);
                        }
                    }

                  CloseHandle (fh);
                }
            }

          if (!skip)
            {
              lf++;
              lb += sz;

              if (o.verbose)
                printf ("%-10llu %s\n", (unsigned long long)sz, fp);
            }
        }
    }
  while (FindNextFileA (h, &fd));
  FindClose (h);

#else
  DIR *dir = opendir (p);
  if (!dir)
    return;

  struct dirent *e;

  while ((e = readdir (dir)))
    {
      const char *n = e->d_name;
      if (n[0] == '.' && (!n[1] || (n[1] == '.' && !n[2])))
        continue;

      char fp[PATH_MAX];
      path_join (fp, sizeof (fp), p, n);
      if (ex_has (ex, n) || ex_has (ex, fp))
        continue;

      struct stat st;
      if (lstat (fp, &st))
        continue;

      if (S_ISDIR (st.st_mode))
        {
#if defined(_OPENMP) && !defined(_MSC_VER)
          if (d < MAX_TASK_DEPTH)
            {
#pragma omp task firstprivate(fp, d, o) shared(ex, ht)
              scan (fp, ex, o, ht, d + 1);
            }
          else
#endif
            scan (fp, ex, o, ht, d + 1);
        }
      else if (S_ISREG (st.st_mode) || S_ISLNK (st.st_mode))
        {
          uint64_t sz = o.disk_usage ? (uint64_t)st.st_blocks * 512
                                     : (uint64_t)st.st_size;
          bool skip = false;

          if (o.disk_usage && ht && S_ISREG (st.st_mode) && st.st_nlink > 1)
            {
              UhashKey k;
              uhash_key_from_stat (&st, &k);

#ifdef _OPENMP
#pragma omp critical
#endif
              skip = uhash_has (ht, k) || !uhash_add (ht, k);
            }

          if (!skip)
            {
              lf++;
              lb += sz;

              if (o.verbose)
                printf ("%-10llu %s\n", (unsigned long long)sz, fp);
            }
        }
    }
  closedir (dir);
#endif

  add_dirs (ld);
  add_files (lf);
  add_bytes (lb);
}

int
main (int argc, char **argv)
{
  Excludes ex = { 0 };
  Opts opts = { .disk_usage = true };
  char *target = NULL;

  for (int i = 1; i < argc; i++)
    {
      char *a = argv[i];

      if (!strncmp (a, "-ex=", 4))
        {
          if (!ex_add (&ex, a + 4))
            return 1;
        }
      else if (!strcmp (a, "-sb") || !strcmp (a, "--size-apparent"))
        {
          opts.disk_usage = false;
        }
      else if (!strcmp (a, "-v") || !strcmp (a, "--verbose"))
        {
          opts.verbose = true;
        }
      else if (!strcmp (a, "-q") || !strcmp (a, "--quiet"))
        {
          opts.quiet = true;
        }
      else if (!strcmp (a, "-h") || !strcmp (a, "--help"))
        {
          printf ("%s\n\n%s", USAGE, LICENSE);
          ex_free (&ex);
          return 0;
        }
      else if (!strcmp (a, "--version"))
        {
          printf ("UDU %s\n%s", VERSION, LICENSE);
          ex_free (&ex);
          return 0;
        }
      else if (a[0] != '-' && !target)
        {
          target = a;
        }
      else
        {
          fprintf (stderr, "udu: Unknown option: %s\n\n%s", a, USAGE);
          ex_free (&ex);
          return 1;
        }
    }

  if (!target)
    {
      fprintf (stderr, "udu: Missing path\n\n%s", USAGE);
      ex_free (&ex);
      return 1;
    }

  Uhash *ht = opts.disk_usage ? uhash_init (0) : NULL;

#if defined(_OPENMP) && !defined(_MSC_VER)
  omp_set_num_threads (omp_get_num_procs () > 0
                           ? omp_get_num_procs ()
                           : 4); // fallback to 4 threads if faild

#pragma omp parallel
#pragma omp single nowait
#endif
  scan (target, &ex, opts, ht, 0);

  if (!opts.quiet)
    {
      char ds[64], fs[64], bs[64], hs[64];
      fmt_comma (ds, load_dirs ());
      fmt_comma (fs, load_files ());
      fmt_comma (bs, load_bytes ());
      human_size (hs, load_bytes ());

      printf ("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
              "  Mode   : %s\n"
              "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
              "  Dirs   : %s\n"
              "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
              "  Files  : %s\n"
              "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
              "  Total  : %s (%s)\n"
              "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n",
              opts.disk_usage ? "Disk Usage" : "Apparent Size", ds, fs, bs,
              hs);
    }

  if (ht)
    uhash_nuke (ht);

  ex_free (&ex);
  return 0;
}
