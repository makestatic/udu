#include "util.h"
#include <stdio.h>
#include <stdlib.h>

// *readablelity*
#define UC(s) ((const unsigned char *)(s))

static bool match_class(const unsigned char **patp, unsigned char c)
{
    const unsigned char *p = *patp;
    bool negate = (*p == '!' || *p == '^');
    if (negate) p++;

    bool matched = false;
    unsigned char prev = 0;
    bool have_prev = false;

    while (*p && *p != ']')
    {
        if (*p == '-' && have_prev && p[1] && p[1] != ']')
        {
            if (prev <= c && c <= p[1]) matched = true;
            p += 2;
            have_prev = false;
        }
        else
        {
            if (*p == c) matched = true;
            prev = *p++;
            have_prev = true;
        }
    }

    if (*p == ']') p++;
    *patp = p;
    return negate ? !matched : matched;
}

static bool glob_match_impl(const unsigned char *pat, const unsigned char *txt)
{
    while (*pat)
    {
        if (*pat == '*')
        {
            while (*pat == '*') pat++;
            if (!*pat) return true;

            for (const unsigned char *t = txt; *t; t++)
            {
                if (glob_match_impl(pat, t)) return true;
            }
            return false;
        }
        else if (*pat == '?')
        {
            if (!*txt) return false;
            pat++;
            txt++;
        }
        else if (*pat == '[')
        {
            if (!*txt) return false;
            pat++;
            if (!match_class(&pat, *txt)) return false;
            txt++;
        }
        else
        {
            if (*pat != *txt) return false;
            pat++;
            txt++;
        }
    }
    return *txt == '\0';
}

bool glob_match(const char *pattern, const char *text)
{
    return pattern && text && glob_match_impl(UC(pattern), UC(text));
}

char *path_join(const char *parent, const char *child)
{
    if (!parent || !child) return NULL;

    size_t plen = strlen(parent);
    size_t clen = strlen(child);

#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    bool has_sep =
      plen > 0 && (parent[plen - 1] == '/' || parent[plen - 1] == '\\');
    size_t total = plen + (has_sep ? 0 : 1) + clen + 1;

    char *out = malloc(total);
    if (!out) return NULL;

    memcpy(out, parent, plen);
    if (!has_sep)
    {
        out[plen] = sep;
        memcpy(out + plen + 1, child, clen + 1);
    }
    else
    {
        memcpy(out + plen, child, clen + 1);
    }

    return out;
}

const char *path_basename(const char *path)
{
    if (!path || !*path) return "";

    const char *last = path;
    for (const char *p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
        {
            last = p + 1;
        }
    }
    return last;
}

char *human_size(uint64_t bytes, char *buf, size_t buflen)
{
    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
    static const size_t unit_count = sizeof(units) / sizeof(units[0]);

    size_t unit = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit < unit_count - 1)
    {
        size /= 1024.0;
        unit++;
    }

    snprintf(buf, buflen, "%.2f%s", size, units[unit]);
    return buf;
}
