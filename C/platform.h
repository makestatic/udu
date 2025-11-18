#ifndef UDU_PLATFORM_H
#define UDU_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "util.h"

#ifndef PATH_MAX
#ifdef _WIN32
#define PATH_MAX 260
#else
#define PATH_MAX 4096
#endif
#endif

void platform_add_directories (size_t n);
void platform_add_files (uint64_t n);
void platform_add_bytes (uint64_t n);
size_t platform_load_directories (void);
uint64_t platform_load_files (void);
uint64_t platform_load_bytes (void);

void platform_path_join (char *destination, size_t size, const char *base,
                         const char *name);

uint64_t platform_calculate_size (const char *path, uint64_t apparent_size,
                                  bool use_disk_usage);

void platform_scan_directory (const char *path, const excludes_list *excludes,
                              options opts, uhash_table *hash_table,
                              int depth);

#endif
