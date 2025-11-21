#ifndef UDU_ARGS_H
#define UDU_ARGS_H

#include <stdbool.h>

typedef struct
{
    char **paths;
    int path_count;
    char **excludes;
    int exclude_count;
    bool apparent_size;
    bool verbose;
    bool quiet;
    bool help;
    bool version;
} args_t;

void args_init(args_t *args);
void args_free(args_t *args);
bool args_parse(args_t *args, int argc, char **argv);
void args_print_help(void);
void args_print_version(void);

#endif
