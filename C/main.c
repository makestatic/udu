#include "args.h"
#include "util.h"
#include "walk.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    args_t args;
    args_init(&args);

    if (!args_parse(&args, argc, argv))
    {
        args_free(&args);
        return 1;
    }

    if (args.help)
    {
        args_print_help();
        args_free(&args);
        return 0;
    }

    if (args.version)
    {
        args_print_version();
        args_free(&args);
        return 0;
    }

    walk_result_t result = walk_paths(args.paths,
                                      args.path_count,
                                      args.excludes,
                                      args.exclude_count,
                                      args.apparent_size,
                                      args.verbose,
                                      args.quiet);

    char size_str[32];
    printf("\nTotal: %s (%lu files, %lu directories)\n",
           human_size(result.total_size, size_str, sizeof(size_str)),
           result.file_count,
           result.dir_count);

    args_free(&args);
    return 0;
}
