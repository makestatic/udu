#ifndef VERSION
    #define VERSION "unknown"
#endif

static const char *USAGE =
  "Usage: udu [option(s)]... [path(s)]...\n"
  " extremely fast disk usage analyzer with parallel traversal engine.\n\n"

  " OPTIONS:\n"
  "  -a, --apparent-size    show file sizes instead of disk usage\n"
  "                          (apparent = bytes reported by filesystem,\n"
  "                           disk usage = actual space allocated)\n"
  "  -h, --help             display this help and exit\n"
  "  -q, --quiet            display output at program exit (default)\n"
  "  -v, --verbose          display each processed file\n"
  "      --version          display version info and exit\n"
  "  -X, --exclude=PATTERN  skip files or directories that match glob pattern\n"
  "                          *        any characters\n"
  "                          ?        single character\n"
  "                          [abc]    any char in set\n"
  "                          Examples: '*.log', 'temp?', '[0-9]*'\n"
  " EXAMPLE:\n"
  "  udu ~/ -avX epstein-files\n\n"
  "Report bugs to <https://github.com/makestatic/udu/issues>\n";

static const char *LICENSE =
  "Copyright (C) 2023, 2024, 2025  Ali Almalki.\n"
  "License GPLv3+: GNU GPL version 3 or later "
  "<https://gnu.org/licenses/gpl.html>\n"
  "This is free software: you are free to change and redistribute it.\n"
  "There is NO WARRANTY, to the extent permitted by law.\n";
