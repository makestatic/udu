#ifndef UDU_TYPES_H
#define UDU_TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
  char **items;
  size_t length;
  size_t capacity;
} excludes_list;

typedef struct
{
  bool verbose;
  bool disk_usage;
  bool quiet;
} options;

#endif
