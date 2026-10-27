#include "app_paths.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define APP_PATH_MAX       300
#define APP_PATH_POOL_NUM  8

static char exe_dir_cache[APP_PATH_MAX] = "";
static int exe_dir_resolved = 0;

static char path_pool[APP_PATH_POOL_NUM][APP_PATH_MAX];
static unsigned int path_idx = 0;

const char *app_exe_dir(void)
{
    char exe_path[APP_PATH_MAX];
    char *slash;
    ssize_t n;

    if (exe_dir_resolved)
        return exe_dir_cache[0] ? exe_dir_cache : NULL;
    exe_dir_resolved = 1;

    n = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (n <= 0)
        return NULL;
    exe_path[n] = '\0';

    slash = strrchr(exe_path, '/');
    if (slash == NULL)
        return NULL;
    slash[1] = '\0';

    snprintf(exe_dir_cache, sizeof(exe_dir_cache), "%s", exe_path);
    return exe_dir_cache;
}

const char *app_resolve_path(const char *path)
{
    char *out;
    const char *dir;

    if (path == NULL || strncmp(path, "./", 2) != 0)
        return path;

    dir = app_exe_dir();
    if (dir == NULL)
        return path;

    out = path_pool[path_idx];
    path_idx = (path_idx + 1) % APP_PATH_POOL_NUM;

    snprintf(out, APP_PATH_MAX, "%s%s", dir, path + 2);
    return out;
}
