#include "args.h"
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2

static bool ensure_capacity(char ***array, int *capacity, int count)
{
    if (count >= *capacity)
    {
        int new_capacity = *capacity * GROWTH_FACTOR;
        char **new_array = realloc(*array, new_capacity * sizeof(char *));
        if (!new_array) return false;
        *array = new_array;
        *capacity = new_capacity;
    }
    return true;
}

void args_init(args_t *args)
{
    memset(args, 0, sizeof(args_t));
}

void args_free(args_t *args)
{
    free(args->paths);
    free(args->excludes);
    memset(args, 0, sizeof(args_t));
}

void args_print_help(void)
{
    printf("%s", USAGE);
}

void args_print_version(void)
{
    printf("UDU %s\n%s", VERSION, LICENSE);
}

static bool handle_short_option(char opt,
                                args_t *args,
                                int *i,
                                int argc,
                                char **argv,
                                int *exclude_capacity)
{
    switch (opt)
    {
        case 'h':
            args->help = true;
            return true;
        case 'a':
            args->apparent_size = true;
            return true;
        case 'v':
            args->verbose = true;
            args->quiet = false;
            return true;
        case 'q':
            args->quiet = true;
            args->verbose = false;
            return true;
        case 'X':
            if (*i + 1 >= argc)
            {
                fprintf(stderr, "Error: -X requires a pattern argument\n");
                return false;
            }
            if (!ensure_capacity(
                  &args->excludes, exclude_capacity, args->exclude_count))
            {
                return false;
            }
            args->excludes[args->exclude_count++] = argv[++(*i)];
            return true;
        default:
            fprintf(stderr, "Error: unknown option '-%c'\n", opt);
            return false;
    }
}

bool args_parse(args_t *args, int argc, char **argv)
{
    int path_capacity = INITIAL_CAPACITY;
    int exclude_capacity = INITIAL_CAPACITY;

    args->paths = malloc(path_capacity * sizeof(char *));
    args->excludes = malloc(exclude_capacity * sizeof(char *));

    if (!args->paths || !args->excludes)
    {
        args_free(args);
        return false;
    }

    args->quiet = true;

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (arg[0] != '-' || arg[1] == '\0')
        {
            // not option; <path>
            if (!ensure_capacity(
                  &args->paths, &path_capacity, args->path_count))
            {
                return false;
            }
            args->paths[args->path_count++] = argv[i];
        }
        else if (arg[1] == '-')
        {
            // full options
            if (strcmp(arg, "--help") == 0)
            {
                args->help = true;
            }
            else if (strcmp(arg, "--version") == 0)
            {
                args->version = true;
            }
            else if (strcmp(arg, "--apparent-size") == 0)
            {
                args->apparent_size = true;
            }
            else if (strcmp(arg, "--verbose") == 0)
            {
                args->verbose = true;
                args->quiet = false;
            }
            else if (strcmp(arg, "--quiet") == 0)
            {
                args->quiet = true;
                args->verbose = false;
            }
            else if (strncmp(arg, "--exclude=", 10) == 0)
            {
                if (!ensure_capacity(
                      &args->excludes, &exclude_capacity, args->exclude_count))
                {
                    return false;
                }
                args->excludes[args->exclude_count++] = (char *)(arg + 10);
            }
            else
            {
                fprintf(stderr, "Error: unknown option '%s'\n", arg);
                return false;
            }
        }
        else
        {
            // handle GNU style options e.g. -avq
            for (int j = 1; arg[j] != '\0'; j++)
            {
                if (!handle_short_option(
                      arg[j], args, &i, argc, argv, &exclude_capacity))
                {
                    return false;
                }
                // If handled -X ; it consumed the next arg so break
                if (arg[j] == 'X')
                {
                    break;
                }
            }
        }
    }

    if (args->help || args->version)
    {
        return true;
    }

    if (args->path_count == 0)
    {
        args->paths[0] = ".";
        args->path_count = 1;
    }

    return true;
}
