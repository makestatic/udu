#include "platform.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

    #include <stdio.h>
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #define PATH_BUFFER_SIZE (MAX_PATH * 3)

struct platform_dir
{
    HANDLE handle;
    WIN32_FIND_DATAW find_data;
    bool first;
    char name_buffer[PATH_BUFFER_SIZE];
};

static bool get_compressed_size(const wchar_t *wpath, uint64_t *size)
{
    DWORD high, low;
    low = GetCompressedFileSizeW(wpath, &high);
    if (low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
    {
        return false;
    }
    ULARGE_INTEGER compressed;
    compressed.LowPart = low;
    compressed.HighPart = high;
    *size = compressed.QuadPart;
    return true;
}

bool platform_stat(const char *path, platform_stat_t *st)
{
    wchar_t wpath[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH) == 0)
    {
        return false;
    }

    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &attr))
    {
        return false;
    }

    st->is_directory = (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    ULARGE_INTEGER size;
    size.LowPart = attr.nFileSizeLow;
    size.HighPart = attr.nFileSizeHigh;
    st->size_apparent = size.QuadPart;

    if (!get_compressed_size(wpath, &st->size_allocated))
    {
        st->size_allocated = st->size_apparent;
    }

    return true;
}

bool platform_is_directory(const char *path)
{
    wchar_t wpath[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH) == 0)
    {
        return false;
    }
    DWORD attr = GetFileAttributesW(wpath);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t platform_file_size(const char *path, bool apparent)
{
    platform_stat_t st;
    return platform_stat(path, &st)
             ? (apparent ? st.size_apparent : st.size_allocated)
             : 0;
}

platform_dir_t *platform_opendir(const char *path)
{
    platform_dir_t *dir = malloc(sizeof(platform_dir_t));
    if (!dir) return NULL;

    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    wchar_t wsearch[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, search_path, -1, wsearch, MAX_PATH) ==
        0)
    {
        free(dir);
        return NULL;
    }

    dir->handle = FindFirstFileW(wsearch, &dir->find_data);
    if (dir->handle == INVALID_HANDLE_VALUE)
    {
        free(dir);
        return NULL;
    }

    dir->first = true;
    return dir;
}

const char *platform_readdir(platform_dir_t *dir)
{
    if (!dir) return NULL;

    while (true)
    {
        if (dir->first)
        {
            dir->first = false;
        }
        else if (!FindNextFileW(dir->handle, &dir->find_data))
        {
            return NULL;
        }

        if (WideCharToMultiByte(CP_UTF8,
                                0,
                                dir->find_data.cFileName,
                                -1,
                                dir->name_buffer,
                                sizeof(dir->name_buffer),
                                NULL,
                                NULL) == 0)
        {
            continue;
        }

        const char *name = dir->name_buffer;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        {
            continue;
        }

        return name;
    }
}

void platform_closedir(platform_dir_t *dir)
{
    if (dir)
    {
        if (dir->handle != INVALID_HANDLE_VALUE)
        {
            FindClose(dir->handle);
        }
        free(dir);
    }
}

bool is_symlink(const char *path)
{
    wchar_t wpath[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH) == 0)
    {
        return false;
    }

    DWORD attrs = GetFileAttributesW(wpath);
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

#else // POSIX

    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>

    #define BLOCK_SIZE 512

struct platform_dir
{
    DIR *dir;
    struct dirent *entry;
};

bool platform_stat(const char *path, platform_stat_t *st)
{
    struct stat sb;
    if (stat(path, &sb) != 0)
    {
        return false;
    }

    st->is_directory = S_ISDIR(sb.st_mode);
    st->size_apparent = (uint64_t)sb.st_size;

    #if defined(__APPLE__) || defined(__linux__)
    st->size_allocated = (uint64_t)sb.st_blocks * BLOCK_SIZE;
    #else
    st->size_allocated = st->size_apparent;
    #endif

    return true;
}

bool platform_is_directory(const char *path)
{
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

uint64_t platform_file_size(const char *path, bool apparent)
{
    platform_stat_t st;
    return platform_stat(path, &st)
             ? (apparent ? st.size_apparent : st.size_allocated)
             : 0;
}

platform_dir_t *platform_opendir(const char *path)
{
    platform_dir_t *dir = malloc(sizeof(platform_dir_t));
    if (!dir) return NULL;

    dir->dir = opendir(path);
    if (!dir->dir)
    {
        free(dir);
        return NULL;
    }

    dir->entry = NULL;
    return dir;
}

const char *platform_readdir(platform_dir_t *dir)
{
    if (!dir || !dir->dir) return NULL;

    while ((dir->entry = readdir(dir->dir)) != NULL)
    {
        const char *name = dir->entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        {
            continue;
        }
        return name;
    }

    return NULL;
}

void platform_closedir(platform_dir_t *dir)
{
    if (dir)
    {
        if (dir->dir) closedir(dir->dir);
        free(dir);
    }
}

bool is_symlink(const char *path)
{
    struct stat st;
    if (lstat(path, &st) != 0) return false;
    return S_ISLNK(st.st_mode);
}
#endif
