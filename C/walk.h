#ifndef WALK_H
#define WALK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint64_t total_size;
    uint64_t file_count;
    uint64_t dir_count;
} walk_result_t;

walk_result_t walk_paths(char **paths,
                         int path_count,
                         char **excludes,
                         int exclude_count,
                         bool apparent_size,
                         bool verbose,
                         bool quiet);

#endif
