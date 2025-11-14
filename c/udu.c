/* Copyright (C) 2023, 2024, 2025 Ali Almalki <github@makestatic>
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

#if defined(_WIN32) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#else
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_OPENMP) && !defined(_MSC_VER)
#include <omp.h>
#endif

#if defined(_WIN32) || defined(WIN32)
#include <intrin.h>
#include <windows.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#ifdef _MSC_VER
#define strdup _strdup
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

static const char* USAGE = "Usage: udu <path> [-ex=<name|path>] [-v|--verbose] "
                           "[-q|--quiet] [-h|--help] [--version]\n\n"
                           "OPTIONS:\n"
                           "  -ex=<name>      Exclude file or directory\n"
                           "  -v, --verbose   Verbose output\n"
                           "  -q, --quiet     Quiet output (default)\n"
                           "  -h, --help      Show help message\n"
                           "  --version       Show version\n";

static const char* LICENSE =
  "Copyright (C) 2023, 2024, 2025  Ali Almalki <github@makestatic>\n"
  "This program comes with ABSOLUTELY NO WARRANTY.\n"
  "This is free software, and you are welcome to redistribute it\n"
  "under the terms of the GNU General Public License.\n";

static const char* VERSION = "2.0.2";

#define MAX_TASK_DEPTH 64
#define EX_INIT_CAPACITY 4

#if defined(_WIN32) && defined(_MSC_VER)

static volatile long long dirs = 0, files = 0, bytes = 0;

static inline void add_dirs(size_t n)
{
    _InterlockedExchangeAdd64(&dirs, (long long)n);
}
static inline void add_files(uint64_t n)
{
    _InterlockedExchangeAdd64(&files, (long long)n);
}
static inline void add_bytes(uint64_t n)
{
    _InterlockedExchangeAdd64(&bytes, (long long)n);
}
static inline size_t load_dirs(void)
{
    return (size_t)_InterlockedOr64(&dirs, 0);
}
static inline uint64_t load_files(void)
{
    return (uint64_t)_InterlockedOr64(&files, 0);
}
static inline uint64_t load_bytes(void)
{
    return (uint64_t)_InterlockedOr64(&bytes, 0);
}

#else

static _Atomic size_t dirs = 0;
static _Atomic uint64_t files = 0, bytes = 0;

static inline void add_dirs(size_t n)
{
    atomic_fetch_add_explicit(&dirs, n, memory_order_relaxed);
}
static inline void add_files(uint64_t n)
{
    atomic_fetch_add_explicit(&files, n, memory_order_relaxed);
}
static inline void add_bytes(uint64_t n)
{
    atomic_fetch_add_explicit(&bytes, n, memory_order_relaxed);
}
static inline size_t load_dirs(void)
{
    return atomic_load(&dirs);
}
static inline uint64_t load_files(void)
{
    return atomic_load(&files);
}
static inline uint64_t load_bytes(void)
{
    return atomic_load(&bytes);
}

#endif

typedef struct
{
    char** items;
    size_t len, cap;
} Excludes;

typedef struct
{
    bool verbose, quiet, help, version;
} Opts;

typedef struct
{
    uint16_t rows, cols;
} Term;

static bool get_term(Term* t)
{
    if (!t) return false;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
    {
        t->rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        t->cols = info.srWindow.Right - info.srWindow.Left + 1;
        return true;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
    {
        t->rows = ws.ws_row ? ws.ws_row : 24;
        t->cols = ws.ws_col ? ws.ws_col : 80;
        return true;
    }
#endif

    t->rows = 24;
    t->cols = 80;
    return false;
}

static void human_size(char* buf, size_t size, uint64_t val)
{
    static const char* units[] = { "B", "K", "M", "G", "T", "P" };
    double num = (double)val;
    int idx = 0;
    while (num >= 1024.0 && idx < 5)
    {
        num /= 1024;
        idx++;
    }
    if (idx == 0)
        snprintf(buf, size, "%llu%s", (unsigned long long)val, units[idx]);
    else
        snprintf(buf, size, "%.1f%s", num, units[idx]);
}

static void fmt_comma(char* buf, size_t size, uint64_t val)
{
    if (!buf || size == 0) return;
    if (val == 0)
    {
        snprintf(buf, size, "0");
        return;
    }

    char tmp[32];
    int pos = 0;
    while (val > 0 && pos < 31)
    {
        tmp[pos++] = '0' + val % 10;
        val /= 10;
    }

    char res[64];
    int rpos = 0;
    for (int i = pos - 1; i >= 0; i--)
    {
        if (rpos > 0 && (pos - i - 1) % 3 == 0) res[rpos++] = ',';
        res[rpos++] = tmp[i];
    }
    res[rpos] = 0;
    snprintf(buf, size, "%s", res);
}

static void ex_init(Excludes* ex)
{
    if (!ex) return;
    ex->items = NULL;
    ex->len = 0;
    ex->cap = 0;
}
static void ex_free(Excludes* ex)
{
    if (!ex) return;
    for (size_t i = 0; i < ex->len; i++) free(ex->items[i]);
    free(ex->items);
    ex->items = NULL;
    ex->len = 0;
    ex->cap = 0;
}

static bool ex_add(Excludes* ex, const char* val)
{
    if (!ex || !val) return false;
    if (ex->len >= ex->cap)
    {
        size_t new_cap = ex->cap ? ex->cap * 2 : EX_INIT_CAPACITY;
        char** new_items = (char**)realloc(ex->items, new_cap * sizeof(char*));
        if (!new_items) return false;
        ex->items = new_items;
        ex->cap = new_cap;
    }
    ex->items[ex->len] = strdup(val);
    if (!ex->items[ex->len]) return false;
    ex->len++;
    return true;
}

static bool ex_has(const Excludes* ex, const char* val)
{
    if (!ex || !val) return false;
    for (size_t i = 0; i < ex->len; i++)
        if (strcmp(ex->items[i], val) == 0) return true;
    return false;
}

static void path_join(char* dst, size_t sz, const char* base, const char* name)
{
    if (!dst || sz == 0 || !base || !name) return;
    if (base[0] == '\0')
    {
        snprintf(dst, sz, "%s", name);
        return;
    }

#ifdef _WIN32
    snprintf(dst, sz, "%s\\%s", base, name);
#else
    size_t len = strlen(base);
    if (base[len - 1] == '/')
        snprintf(dst, sz, "%s%s", base, name);
    else
        snprintf(dst, sz, "%s/%s", base, name);
#endif
}

static void scan_dir(const char* path, const Excludes* ex, Opts opts, int depth)
{
    if (!path || !ex) return;

    size_t local_dirs = 1;
    uint64_t local_files = 0, local_bytes = 0;

#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    char search[PATH_MAX];
    if (snprintf(search, sizeof(search), "%s\\*", path) < 0 ||
        strlen(search) >= sizeof(search))
        return;
    HANDLE h = FindFirstFileA(search, &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do
    {
        const char* name = fd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char full[PATH_MAX];
        path_join(full, sizeof(full), path, name);

        if (ex_has(ex, name) || ex_has(ex, full)) continue;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
#if defined(_OPENMP) && !defined(_MSC_VER)
            if (depth < MAX_TASK_DEPTH)
            {
#pragma omp task firstprivate(full, depth, opts) shared(ex)
                scan_dir(full, ex, opts, depth + 1);
            }
            else
                scan_dir(full, ex, opts, depth + 1);
#else
            scan_dir(full, ex, opts, depth + 1);
#endif
        }
        else
        {
            uint64_t sz = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            local_files++;
            local_bytes += sz;
            if (opts.verbose)
            {
                char tmp[32];
                human_size(tmp, sizeof(tmp), sz);
                printf("%-10s %s\n", tmp, full);
            }
        }

    } while (FindNextFileA(h, &fd) != 0);
    FindClose(h);

#else
    DIR* d = opendir(path);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL)
    {
        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char full[PATH_MAX];
        path_join(full, sizeof(full), path, name);

        if (ex_has(ex, name) || ex_has(ex, full)) continue;

        struct stat st;
        if (lstat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode))
        {
#if defined(_OPENMP) && !defined(_MSC_VER)
            if (depth < MAX_TASK_DEPTH)
            {
#pragma omp task firstprivate(full, depth, opts) shared(ex)
                scan_dir(full, ex, opts, depth + 1);
            }
            else
                scan_dir(full, ex, opts, depth + 1);
#else
            scan_dir(full, ex, opts, depth + 1);
#endif
        }
        else if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))
        {
            uint64_t sz = (uint64_t)st.st_size;
            local_files++;
            local_bytes += sz;
            if (opts.verbose)
            {
                char tmp[32];
                human_size(tmp, sizeof(tmp), sz);
                printf("%-10s %s\n", tmp, full);
            }
        }
    }
    closedir(d);
#endif

    add_dirs(local_dirs);
    add_files(local_files);
    add_bytes(local_bytes);
}

static bool parse_args(int argc,
                       char** argv,
                       char** target,
                       Excludes* exs,
                       Opts* opts)
{
    if (!argv || !target || !exs || !opts) return false;

    opts->verbose = opts->quiet = opts->help = opts->version = false;
    *target = NULL;

    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        if (strncmp(arg, "-ex=", 4) == 0)
        {
            if (!ex_add(exs, arg + 4))
            {
                fprintf(stderr, "OOM\n");
                return false;
            }
        }
        else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0)
            opts->verbose = true;
        else if (strcmp(arg, "-q") == 0 || strcmp(arg, "--quiet") == 0)
            opts->quiet = true;
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
            opts->help = true;
        else if (strcmp(arg, "--version") == 0)
            opts->version = true;
        else if (arg[0] != '-' && *target == NULL)
            *target = argv[i];
        else
        {
            fprintf(stderr, "Unknown '%s'\n\n%s", arg, USAGE);
            return false;
        }
    }

    if (*target == NULL && !opts->help && !opts->version)
    {
        fprintf(stderr, "Missing path\n\n%s", USAGE);
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    Excludes exs;
    ex_init(&exs);
    Opts opts;
    char* target = NULL;

    if (!parse_args(argc, argv, &target, &exs, &opts))
    {
        ex_free(&exs);
        return 1;
    }

    if (opts.help)
    {
        printf("%s\n%s", USAGE, LICENSE);
        ex_free(&exs);
        return 0;
    }

    if (opts.version)
    {
        printf("udu v%s\n\n%s", VERSION, LICENSE);
        ex_free(&exs);
        return 0;
    }

    Term term;
    get_term(&term);

#if defined(_OPENMP) && !defined(_MSC_VER)
    int nthreads = omp_get_num_procs();
    if (nthreads <= 0) nthreads = 4;
    omp_set_num_threads(nthreads);
#endif

#if defined(_OPENMP) && !defined(_MSC_VER)
#pragma omp parallel
#pragma omp single nowait
#endif
    scan_dir(target, &exs, opts, 0);

    size_t total_dirs = load_dirs();
    uint64_t total_files = load_files(), total_bytes = load_bytes();

    char dir_str[64], file_str[64], byte_str[64], human_str[64];
    fmt_comma(dir_str, sizeof(dir_str), total_dirs);
    fmt_comma(file_str, sizeof(file_str), total_files);
    fmt_comma(byte_str, sizeof(byte_str), total_bytes);
    human_size(human_str, sizeof(human_str), total_bytes);

    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
           "  Dirs   : %s\n"
           "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
           "  Files  : %s\n"
           "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
           "  Total  : %s (%s)\n"
           "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n",
           dir_str,
           file_str,
           byte_str,
           human_str);

    ex_free(&exs);
    return 0;
}
