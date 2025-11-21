#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

bool glob_match(const char *pattern, const char *text);
char *path_join(const char *parent, const char *child);
const char *path_basename(const char *path);
char *human_size(uint64_t bytes, char *buf, size_t buflen);

#endif
