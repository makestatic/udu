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
#define _CRT_SECURE_NO_WARNINGS // shuting up on win
#else
// enable gnu extensions
#define _GNU_SOURCE
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#ifdef _MSC_VER
#define strdup _strdup
#endif
#endif

#if defined(_OPENMP) && !defined(_MSC_VER)
#include <omp.h>
#endif

#include "platform.h"

#ifndef VERSION
#define VERSION "unknown"
#endif

typedef struct
{
  char **items;
  size_t length;
  size_t capacity;
} targets_list_t;

static const char *USAGE
    = "Usage: udu <path>... [OPTIONS]...\n"
      "Summarize device usage of FILES or DIRECTORIES, recursively\n\n"
      "  -a, --apparent-size            display apparent sizes rather than "
      "device "
      "usage\n"
      "  -v, --verbose                  enable verbose output\n"
      "  -q, --quiet                    enable quiet output (default)\n"
      "  -X <name>, --exclude=<name>    exclude file or directory\n"
      "  -h, --help                     display this help and exit\n"
      "      --version                  display version information and "
      "exit\n\n"
      "Report bugs to: <https://github.com/makestatic/udu/issues>\n"
      "UDU home page: <https://github.com/makestatic/udu>\n";

static const char *LICENSE
    = "Copyright (C) 2023, 2024, 2025  Ali Almalki.\n"
      "License GPLv3+: GNU GPL version 3 or later "
      "<https://gnu.org/licenses/gpl.html>\n"
      "This is free software: you are free to change and redistribute it.\n"
      "There is NO WARRANTY, to the extent permitted by law.\n";

static bool
targets_add (targets_list_t *t, char *value)
{
  if (!t || !value)
    return false;
  if (t->length >= t->capacity)
    {
      size_t new_capacity = t->capacity ? t->capacity << 1 : 4;
      char **new_items = realloc (t->items, new_capacity * sizeof (char *));
      if (!new_items)
        return false;
      t->items = new_items;
      t->capacity = new_capacity;
    }
  t->items[t->length++] = value;
  return true;
}

static int
handle_help_version (const char *argument, excludes_list *excludes,
                     targets_list_t *targets)
{
  if (!strcmp (argument, "-h") || !strcmp (argument, "--help"))
    {
      printf ("%s\n\n%s", USAGE, LICENSE);
      excludes_free (excludes);
      free (targets->items);
      return 0;
    }
  else if (!strcmp (argument, "--version"))
    {
      printf ("UDU %s\n%s", VERSION, LICENSE);
      excludes_free (excludes);
      free (targets->items);
      return 0;
    }
  return -1;
}

static int
parse_exclude_option (char *argument, int *i, int argc, char **argv,
                      excludes_list *excludes, targets_list_t *targets)
{
  if (!strncmp (argument, "--exclude=", 10))
    {
      const char *val = argument + 10;
      if (!*val)
        {
          fprintf (stderr, "udu: --exclude requires a non-empty argument\n");
          excludes_free (excludes);
          free (targets->items);
          return 1;
        }
      if (!excludes_add (excludes, val))
        {
          fprintf (stderr, "udu: failed to add --exclude value\n");
          excludes_free (excludes);
          free (targets->items);
          return 1;
        }
    }
  else if (!strcmp (argument, "-X"))
    {
      if (*i + 1 >= argc)
        {
          fprintf (stderr, "udu: option '-X' requires an argument\n");
          excludes_free (excludes);
          free (targets->items);
          return 1;
        }
      (*i)++;
      if (!excludes_add (excludes, argv[*i]))
        {
          fprintf (stderr, "udu: failed to add -X value\n");
          excludes_free (excludes);
          free (targets->items);
          return 1;
        }
    }
  return -1;
}

static void
print_summary (void)
{
  char directories_formatted[64], files_formatted[64], bytes_formatted[64],
      human_size[64];
  fmt_kcomma (directories_formatted, platform_load_directories ());
  fmt_kcomma (files_formatted, platform_load_files ());
  fmt_kcomma (bytes_formatted, platform_load_bytes ());
  human_readable (human_size, platform_load_bytes ());

  printf ("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
          " Dirs: %s\n"
          "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
          " Files: %s\n"
          "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
          " Total: %s (%s)\n\n",
          directories_formatted, files_formatted, bytes_formatted, human_size);
}

int
main (int argc, char **argv)
{
  excludes_list excludes = { 0 };
  options opts = { .disk_usage = true, .verbose = false, .quiet = false };
  targets_list_t targets = { 0 };

  if (argc < 1)
    return 1;

  for (int i = 1; i < argc; ++i)
    {
      char *argument = argv[i];

      int result = handle_help_version (argument, &excludes, &targets);
      if (result != -1)
        return result;

      if (!strcmp (argument, "-v") || !strcmp (argument, "--verbose"))
        opts.verbose = true;
      else if (!strcmp (argument, "-q") || !strcmp (argument, "--quiet"))
        opts.quiet = true;
      else if (!strcmp (argument, "--apparent-size")
               || !strcmp (argument, "-a"))
        opts.disk_usage = false;
      else if (!strncmp (argument, "--exclude=", 10)
               || !strcmp (argument, "-X"))
        {
          result = parse_exclude_option (argument, &i, argc, argv, &excludes,
                                         &targets);
          if (result != -1)
            return result;
        }
      else if (argument[0] == '-')
        {
          fprintf (stderr,
                   "udu: invalid option -- '%s'\nTry 'udu --help' for more "
                   "information.\n",
                   argument);
          excludes_free (&excludes);
          free (targets.items);
          return 1;
        }
      else
        {
          if (!targets_add (&targets, argument))
            {
              fprintf (stderr, "udu: failed to record target path\n");
              excludes_free (&excludes);
              free (targets.items);
              return 1;
            }
        }
    }

  if (targets.length == 0)
    {
      if (!targets_add (&targets, "."))
        {
          fprintf (stderr, "udu: failed to record default target '.'\n");
          excludes_free (&excludes);
          free (targets.items);
          return 1;
        }
      opts.verbose = true;
    }

  uhash_table *hash_table = opts.disk_usage ? uhash_init (0) : NULL;
  if (opts.disk_usage && !hash_table)
    {
      fprintf (stderr, "udu: failed to initialize inode hash table\n");
      excludes_free (&excludes);
      free (targets.items);
      return 1;
    }

#if defined(_OPENMP) && !defined(_MSC_VER)
  omp_set_num_threads (omp_get_num_procs () > 0 ? omp_get_num_procs () : 4);
#pragma omp parallel
#pragma omp single nowait
#endif
  {
    for (size_t t = 0; t < targets.length; ++t)
      platform_scan_directory (targets.items[t], &excludes, opts, hash_table,
                               0);
  }

  if (!opts.quiet)
    print_summary ();

  if (hash_table)
    uhash_nuke (hash_table);

  excludes_free (&excludes);
  free (targets.items);
  return 0;
}
