#include "walk.h"
#include "platform.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _OPENMP
    #include <omp.h>
#endif

#define MAX_SYMLINK_DEPTH 64

typedef struct
{
    char **excludes;
    int exclude_count;
    bool apparent_size;
    bool verbose;
    uint64_t total_size;
    uint64_t file_count;
    uint64_t dir_count;
} walk_context_t;

static inline bool is_excluded(const char *name,
                               const char *fullpath,
                               const walk_context_t *ctx)
{
    for (int i = 0; i < ctx->exclude_count; i++)
    {
        if (glob_match(ctx->excludes[i], name) ||
            glob_match(ctx->excludes[i], fullpath))
        {
            return true;
        }
    }
    return false;
}

static void process_file(const char *fullpath,
                         uint64_t size,
                         walk_context_t *ctx)
{
#pragma omp atomic
    ctx->total_size += size;
#pragma omp atomic
    ctx->file_count++;

    if (ctx->verbose)
    {
#pragma omp critical
        {
            char buf[32];
            printf("%-8s %s\n", human_size(size, buf, sizeof(buf)), fullpath);
        }
    }
}

static void walk_directory_impl(const char *path,
                                walk_context_t *ctx,
                                int depth)
{
    // dodge infinite symlink loops
    if (depth > MAX_SYMLINK_DEPTH)
    {
        if (ctx->verbose)
        {
            fprintf(
              stderr, "Warning: max symlink depth reached at '%s'\n", path);
        }
        return;
    }

    platform_dir_t *dir = platform_opendir(path);
    if (!dir) return;

    const char *entry;
    while ((entry = platform_readdir(dir)) != NULL)
    {
        char *fullpath = path_join(path, entry);
        if (!fullpath) continue;

        if (is_excluded(entry, fullpath, ctx))
        {
            free(fullpath);
            continue;
        }

        if (is_symlink(fullpath))
        {
            free(fullpath);
            continue;
        }

        platform_stat_t st;
        if (!platform_stat(fullpath, &st))
        {
            free(fullpath);
            continue;
        }

        if (st.is_directory)
        {
#pragma omp task firstprivate(fullpath, depth) shared(ctx)
            {
                walk_directory_impl(fullpath, ctx, depth + 1);
                free(fullpath);
            }
#pragma omp atomic
            ctx->dir_count++;
        }
        else
        {
            uint64_t size =
              ctx->apparent_size ? st.size_apparent : st.size_allocated;
            process_file(fullpath, size, ctx);
            free(fullpath);
        }
    }

#pragma omp taskwait
    platform_closedir(dir);
}

static void walk_directory(const char *path, walk_context_t *ctx)
{
    walk_directory_impl(path, ctx, 0);
}

walk_result_t walk_paths(char **paths,
                         int path_count,
                         char **excludes,
                         int exclude_count,
                         bool apparent_size,
                         bool verbose,
                         bool quiet)
{
    walk_context_t ctx = { .excludes = excludes,
                           .exclude_count = exclude_count,
                           .apparent_size = apparent_size,
                           .verbose = verbose,
                           .total_size = 0,
                           .file_count = 0,
                           .dir_count = 0 };

#pragma omp parallel
    {
#pragma omp single nowait
        {
            for (int i = 0; i < path_count; i++)
            {
                const char *path = paths[i];
                platform_stat_t st;

                if (!platform_stat(path, &st))
                {
                    fprintf(stderr, "Error: cannot stat '%s'\n", path);
                    continue;
                }

                if (st.is_directory)
                {
#pragma omp task firstprivate(path) shared(ctx)
                    {
                        walk_directory(path, &ctx);
                    }
#pragma omp atomic
                    ctx.dir_count++;
                }
                else
                {
                    uint64_t size =
                      apparent_size ? st.size_apparent : st.size_allocated;
                    process_file(path, size, &ctx);
                }
            }
        }
    }

    walk_result_t result = { .total_size = ctx.total_size,
                             .file_count = ctx.file_count,
                             .dir_count = ctx.dir_count };
    return result;
}
