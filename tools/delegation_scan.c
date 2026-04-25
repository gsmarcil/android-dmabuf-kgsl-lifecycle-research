#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

/*
 * delegation_scan.c
   * Scans all processes for KGSL/DMA-BUF file descriptors
 * and reports SELinux context + command line for each match.
   */

static void read_cmdline(pid_t pid, char *buf, size_t buflen) {
      if (buflen == 0) return;
    snprintf(buf, buflen, "unknown");

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *f = fopen(path, "r");
    if (!f) return;

    size_t n = fread(buf, 1, buflen - 1, f);
    fclose(f);

    if (n == 0) {
        snprintf(buf, buflen, "unknown");
        return;
    }

    buf[n] = '\0';
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\0') buf[i] = ' ';
    }
}

static void read_context(pid_t pid, char *buf, size_t buflen) {
      if (buflen == 0) return;
    snprintf(buf, buflen, "unknown");

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/attr/current", pid);

    FILE *f = fopen(path, "r");
    if (!f) return;

    if (fgets(buf, (int)buflen, f)) {
        buf[strcspn(buf, "\n")] = '\0';
    } else {
        snprintf(buf, buflen, "unknown");
    }
    fclose(f);
}

static int is_interesting_target(const char *target) {
      return (strstr(target, "kgsl-3d0") != NULL) ||
                   (strstr(target, "dmabuf") != NULL) ||
                   (strstr(target, "dma-buf") != NULL);
}

int main(void) {
      printf("=== GPU Memory Delegation Scanner ===\n");
    printf("Scanning all processes for KGSL/DMA-BUF fds...\n\n");

    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("opendir /proc");
        return 1;
    }

    struct dirent *entry;
    int total_kgsl = 0, total_dmabuf = 0;

    while ((entry = readdir(proc))) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;

        pid_t pid = (pid_t)atoi(entry->d_name);

        char fd_dir[PATH_MAX];
        snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", entry->d_name);

        DIR *fd_d = opendir(fd_dir);
        if (!fd_d) continue;

        struct dirent *fe;
        while ((fe = readdir(fd_d))) {
            if (fe->d_name[0] == '.') continue;

            char link_path[PATH_MAX];
            char target[PATH_MAX];

            snprintf(link_path, sizeof(link_path), "/proc/%s/fd/%s", entry->d_name, fe->d_name);

            ssize_t len = readlink(link_path, target, sizeof(target) - 1);
            if (len <= 0) continue;
            target[len] = '\0';

            if (is_interesting_target(target)) {
                char cmdline[256];
                char ctx[128];
                read_cmdline(pid, cmdline, sizeof(cmdline));
                read_context(pid, ctx, sizeof(ctx));

                printf("PID %-6s | %-45s | %s -> %s\n",
                                         entry->d_name, ctx, cmdline, target);

                if (strstr(target, "kgsl")) total_kgsl++;
                else total_dmabuf++;
            }
        }

        closedir(fd_d);
    }

    closedir(proc);

    printf("\n=== Summary ===\n");
    printf("KGSL users: %d\n", total_kgsl);
    printf("DMA-BUF users: %d\n", total_dmabuf);
    printf("\nLook for: untrusted_app, isolated_app, priv_app, platform_app with dmabuf fds\n");

    return 0;
}
