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

// TODO: docement this so future me doesn't hate past me




#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <dirent.h>
#include <inttypes.h>
#include <limits.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

static const char* usage =
  "Usage: udu <path> [-ex=<name|path>] [-v|--verbose] [-q|--quiet] "
  "[-h|--help] [--version]\n";

static const char* license =
  "udu  Copyright (C) 2023, 2024, 2025  Ali Almalki <github@makestatic>\n"
  "This program comes with ABSOLUTELY NO WARRANTY.\n"
  "This is free software, and you are welcome to redistribute it\n"
  "under the terms of the GNU General Public License.\n";

static const char* version = "1.0.10";

struct WinSize
{
    unsigned short ws_row, ws_col;
};

static bool get_terminal_size(struct WinSize* ws)
{
    if (!ws) return false;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        ws->ws_row = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        ws->ws_col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        return true;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
    {
        ws->ws_row = w.ws_row ? w.ws_row : 24;
        ws->ws_col = w.ws_col ? w.ws_col : 80;
        return true;
    }
#endif
    return false;
}

static void human_size(char* buf, size_t bufsz, unsigned long long bytes)
{
    if (!buf || bufsz == 0) return;

    const char* units[] = { "B", "K", "M", "G", "T", "P" };
    double v = (double)bytes;
    int i = 0;
    while (v >= 1024.0 && i < 5)
    {
        v /= 1024.0;
        i++;
    }
    if (i == 0)
        snprintf(buf, bufsz, "%llu%s", bytes, units[i]);
    else
        snprintf(buf, bufsz, "%.1f%s", v, units[i]);
}

static void k_formater(char* buf, size_t bufsz, unsigned long long val)
{
    if (!buf || bufsz == 0) return;

    char tmp[64];
    int pos = 0;
    if (val == 0)
    {
        if (bufsz > 1)
        {
            buf[0] = '0';
            buf[1] = '\0';
        }
        else if (bufsz > 0)
            buf[0] = '\0';
        return;
    }
    while (val > 0 && pos < (int)(sizeof(tmp) - 1))
    {
        tmp[pos++] = '0' + (char)(val % 10);
        val /= 10;
    }
    char out[128];
    int outpos = 0;
    for (int i = 0; i < pos; ++i)
    {
        if (i > 0 && i % 3 == 0 && outpos < (int)(sizeof(out) - 1))
            out[outpos++] = ',';
        if (outpos < (int)(sizeof(out) - 1)) out[outpos++] = tmp[i];
    }
    for (int i = 0; i < outpos / 2; ++i)
    {
        char c = out[i];
        out[i] = out[outpos - 1 - i];
        out[outpos - 1 - i] = c;
    }
    out[outpos] = '\0';
    if (bufsz > 0)
    {
        size_t n = (strlen(out) < bufsz - 1) ? strlen(out) : bufsz - 1;
        memcpy(buf, out, n);
        buf[n] = '\0';
    }
}

typedef struct
{
    char** items;
    size_t count, capacity;
} ExcludeList;

static void exclude_init(ExcludeList* e)
{
    if (!e) return;
    e->items = NULL;
    e->count = e->capacity = 0;
}

static void exclude_free(ExcludeList* e)
{
    if (!e) return;
    for (size_t i = 0; i < e->count; i++)
    {
        free(e->items[i]);
    }
    free(e->items);
    e->items = NULL;
    e->count = e->capacity = 0;
}

static int exclude_add(ExcludeList* e, const char* s)
{
    if (!e || !s) return -1;

    if (e->count + 1 > e->capacity)
    {
        size_t n = e->capacity ? e->capacity * 2 : 4;
        char** t = realloc(e->items, n * sizeof(*t));
        if (!t) return -1;
        e->items = t;
        e->capacity = n;
    }
    e->items[e->count] = strdup(s);
    if (!e->items[e->count]) return -1;
    e->count++;
    return 0;
}

static bool exclude_match_any(const ExcludeList* e, const char* s)
{
    if (!e || !s) return false;

    for (size_t i = 0; i < e->count; i++)
    {
        if (e->items[i] && strcmp(e->items[i], s) == 0) return true;
    }
    return false;
}

static atomic_size_t g_dirs = ATOMIC_VAR_INIT(0);
static atomic_ullong g_files = ATOMIC_VAR_INIT(0);
static atomic_ullong g_size = ATOMIC_VAR_INIT(0);

typedef struct
{
    bool verbose, quiet, help, version;
} Options;

static void join_path(char* out, size_t outlen, const char* a, const char* b)
{
    if (!out || outlen == 0 || !a || !b) return;

    if (a[0] == '\0')
    {
        snprintf(out, outlen, "%s", b);
        return;
    }
#ifdef _WIN32
    snprintf(out, outlen, "%s\\%s", a, b);
#else
    size_t al = strlen(a);
    if (a[al - 1] == '/')
        snprintf(out, outlen, "%s%s", a, b);
    else
        snprintf(out, outlen, "%s/%s", a, b);
#endif
}

#define TASK_DEPTH_THRESHOLD 64 // TODO: more machinery way setting this

static void process_dir(const char* path,
                        const ExcludeList* exs,
                        Options opts,
                        int depth)
{
    if (!path || !exs) return;

#ifdef _WIN32
    WIN32_FIND_DATAA ffd;
    char search_path[PATH_MAX];
    int ret = snprintf(search_path, sizeof(search_path), "%s\\*", path);
    if (ret < 0 || (size_t)ret >= sizeof(search_path)) return;

    HANDLE hFind = FindFirstFileA(search_path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    size_t local_dirs = 1;
    unsigned long long local_files = 0, local_size = 0;

    do
    {
        const char* name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char full[PATH_MAX];
        join_path(full, sizeof(full), path, name);

        if (exclude_match_any(exs, name) || exclude_match_any(exs, full))
            continue;

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (depth < TASK_DEPTH_THRESHOLD)
            {
#pragma omp task firstprivate(full, depth, opts) shared(exs)
                process_dir(full, exs, opts, depth + 1);
            }
            else
                process_dir(full, exs, opts, depth + 1);
        }
        else
        {
            local_files++;
            unsigned long long file_size =
              ((unsigned long long)ffd.nFileSizeHigh << 32) | ffd.nFileSizeLow;
            local_size += file_size;

            if (opts.verbose)
            {
                char sizebuf[32];
                human_size(sizebuf, sizeof(sizebuf), file_size);
                printf("%-8s %s\n", sizebuf, full);
            }
        }
    } while (FindNextFileA(hFind, &ffd) != 0);
    FindClose(hFind);
#else
    DIR* d = opendir(path);
    if (!d) return;

    struct dirent* ent;
    size_t local_dirs = 1;
    unsigned long long local_files = 0, local_size = 0;

    while ((ent = readdir(d)) != NULL)
    {
        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char full[PATH_MAX];
        join_path(full, sizeof(full), path, name);

        if (exclude_match_any(exs, name) || exclude_match_any(exs, full))
            continue;

        struct stat st;
        if (lstat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode))
        {
            if (depth < TASK_DEPTH_THRESHOLD)
            {
#pragma omp task firstprivate(full, depth, opts) shared(exs)
                process_dir(full, exs, opts, depth + 1);
            }
            else
                process_dir(full, exs, opts, depth + 1);
        }
        else if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))
        {
            local_files++;
            local_size += (unsigned long long)st.st_size;

            if (opts.verbose)
            {
                char sizebuf[32];
                human_size(
                  sizebuf, sizeof(sizebuf), (unsigned long long)st.st_size);
                printf("%-8s %s\n", sizebuf, full);
            }
        }
    }
    closedir(d);
#endif
    atomic_fetch_add_explicit(&g_dirs, local_dirs, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_files, local_files, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_size, local_size, memory_order_relaxed);
}

static int parse_args(int argc,
                      char** argv,
                      char** out_path,
                      ExcludeList* exs,
                      Options* opts)
{
    if (!argv || !out_path || !exs || !opts) return -1;

    opts->verbose = opts->quiet = opts->help = opts->version = false;
    *out_path = NULL;

    for (int i = 1; i < argc; i++)
    {
        char* a = argv[i];
        if (!a) continue;

        if (strncmp(a, "-ex=", 4) == 0)
        {
            if (exclude_add(exs, a + 4) != 0)
            {
                fprintf(stderr, "[ERROR] Out of memory\n");
                return -1;
            }
        }
        else if (strcmp(a, "-v") == 0 || strcmp(a, "--verbose") == 0)
            opts->verbose = true;
        else if (strcmp(a, "-q") == 0 || strcmp(a, "--quiet") == 0)
            opts->quiet = true;
        else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0)
            opts->help = true;
        else if (strcmp(a, "--version") == 0 || strcmp(a, "--v") == 0)
            opts->version = true;
        else if (a[0] != '-' && *out_path == NULL)
            *out_path = a;
        else
        {
            fprintf(stderr, "[ERROR] Unknown option: %s\n%s", a, usage);
            return -1;
        }
    }
    if (*out_path == NULL && !opts->help && !opts->version)
    {
        fprintf(stderr, "[ERROR] Missing <path>\n%s", usage);
        return -1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    ExcludeList exs;
    exclude_init(&exs);
    Options opts;
    char* path;

    if (parse_args(argc, argv, &path, &exs, &opts) != 0)
    {
        exclude_free(&exs);
        return 1;
    }
    if (opts.help)
    {
        printf("%s\n%s\n", usage, license);
        exclude_free(&exs);
        return 0;
    }
    if (opts.version)
    {
        printf("version: %s\n%s\n", version, license);
        exclude_free(&exs);
        return 0;
    }

    struct WinSize ws;
    if (!get_terminal_size(&ws))
    {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }

#ifdef _OPENMP
    int cpu_count = omp_get_num_procs();
    int threads = cpu_count > 0 ? cpu_count : 4;
    omp_set_num_threads(threads);
#endif

    atomic_store(&g_dirs, 0);
    atomic_store(&g_files, 0);
    atomic_store(&g_size, 0);

#pragma omp parallel
#pragma omp single nowait
    process_dir(path, &exs, opts, 0);

    unsigned long long total_size = atomic_load(&g_size);
    unsigned long long total_files = atomic_load(&g_files);
    size_t total_dirs = atomic_load(&g_dirs);

    char buf_dirs[64], buf_files[64], buf_size[64], buf_human[64];
    k_formater(buf_dirs, sizeof(buf_dirs), total_dirs);
    k_formater(buf_files, sizeof(buf_files), total_files);
    k_formater(buf_size, sizeof(buf_size), total_size);
    human_size(buf_human, sizeof(buf_human), total_size);

    printf(" ------------------------------------------\n"
           "¦ Directories  ¦  %s\n"
           " ------------------------------------------\n"
           "¦ Files        ¦  %s\n"
           " ------------------------------------------\n"
           "¦ Total        ¦  %s (%s)\n"
           " ------------------------------------------\n",
           buf_dirs,
           buf_files,
           buf_size,
           buf_human);

    exclude_free(&exs);
    return 0;
}
