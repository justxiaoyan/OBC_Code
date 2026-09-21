// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define OBC_SYSINFO_PATH "/proc/obcbase/sysinfo"

static void printUsage(const char *program)
{
    fprintf(stderr, "Usage: %s sysflag -g\n", program);
}

static int copySysinfo(void)
{
    char buffer[512];
    ssize_t count;
    int fd;

    fd = open(OBC_SYSINFO_PATH, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "obctool: cannot open %s: %s\n",
                OBC_SYSINFO_PATH, strerror(errno));
        return 1;
    }

    while ((count = read(fd, buffer, sizeof(buffer))) != 0) {
        ssize_t written = 0;

        if (count < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "obctool: cannot read %s: %s\n",
                    OBC_SYSINFO_PATH, strerror(errno));
            close(fd);
            return 1;
        }

        while (written < count) {
            ssize_t result = write(STDOUT_FILENO, buffer + written,
                                   (size_t)(count - written));

            if (result < 0) {
                if (errno == EINTR)
                    continue;
                fprintf(stderr, "obctool: cannot write output: %s\n",
                        strerror(errno));
                close(fd);
                return 1;
            }
            written += result;
        }
    }

    if (close(fd) != 0) {
        fprintf(stderr, "obctool: cannot close %s: %s\n",
                OBC_SYSINFO_PATH, strerror(errno));
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[1], "sysflag") != 0 ||
        strcmp(argv[2], "-g") != 0) {
        printUsage(argv[0]);
        return 2;
    }

    return copySysinfo();
}
