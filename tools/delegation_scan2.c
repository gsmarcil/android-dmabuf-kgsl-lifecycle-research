```c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

/*
 * delegation_scan2.c
 *
 * Scan /proc and classify processes that currently hold DMA-BUF
 * file descriptors by SELinux domain category.
 *
 * Categories:
 *   [APP]     untrusted_app / isolated_app / priv_app / platform_app
 *   [SYSTEM]  surfaceflinger / system_server / servicemanager / netd / zygote
 *   [VENDOR]  vendor_* / hal_*
 *   [OTHER]   everything else
 */

static int is_numeric(const char *s) {
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

static void read_text_file(const char *path, char *buf, size_t size) {
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(buf, size, "unknown");
        return;
    }

    size_t n = fread(buf, 1, size - 1, f);
    fclose(f);

    if (n == 0) {
        snprintf(buf, size, "unknown");
        return;
    }

    buf[n] = '\0';

    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\0')
            buf[i] = ' ';
    }

    buf[strcspn(buf, "\n")] = 0;
}

static int has_dmabuf_fd(const char *pid) {
    char fd_dir[256];
    snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", pid);

    DIR *d = opendir(fd_dir);
    if (!d) return 0;

    struct dirent *e;
    int found = 0;

    while ((e = readdir(d))) {
        if (e->d_name[0] == '.')
            continue;

        char link_path[512];
        char target[512];

        snprintf(link_path, sizeof(link_path),
                 "/proc/%s/fd/%s", pid, e->d_name);

        ssize_t len = readlink(link_path, target, sizeof(target) - 1);
        if (len <= 0)
            continue;

        target[len] = '\0';

        if (strstr(target, "dmabuf") ||
            strstr(target, "dma-buf") ||
            strstr(target, "anon_inode:[dmabuf]")) {
            found = 1;
            break;
        }
    }

    closedir(d);
    return found;
}

static const char *classify_ctx(const char *ctx) {
    if (strstr(ctx, "untrusted_app") ||
        strstr(ctx, "isolated_app") ||
        strstr(ctx, "priv_app") ||
        strstr(ctx, "platform_app"))
        return "APP";

    if (strstr(ctx, "surfaceflinger") ||
        strstr(ctx, "system_server") ||
        strstr(ctx, "servicemanager") ||
        strstr(ctx, "netd") ||
        strstr(ctx, "zygote"))
        return "SYSTEM";

    if (strstr(ctx, "vendor") ||
        strstr(ctx, "hal_"))
        return "VENDOR";

    return "OTHER";
}

int main(void) {
    printf("=== DMA-BUF Users by SELinux Domain ===\n\n");

    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("opendir /proc");
        return 1;
    }

    int app = 0, system = 0, vendor = 0, other = 0;
    struct dirent *entry;

    while ((entry = readdir(proc))) {
        if (!is_numeric(entry->d_name))
            continue;

        if (!has_dmabuf_fd(entry->d_name))
            continue;

        char ctx_path[256];
        char cmd_path[256];
        char ctx[256];
        char cmd[256];

        snprintf(ctx_path, sizeof(ctx_path),
                 "/proc/%s/attr/current", entry->d_name);

        snprintf(cmd_path, sizeof(cmd_path),
                 "/proc/%s/cmdline", entry->d_name);

        read_text_file(ctx_path, ctx, sizeof(ctx));
        read_text_file(cmd_path, cmd, sizeof(cmd));

        const char *cls = classify_ctx(ctx);

        if (!strcmp(cls, "APP")) app++;
        else if (!strcmp(cls, "SYSTEM")) system++;
        else if (!strcmp(cls, "VENDOR")) vendor++;
        else other++;

        printf("[%-6s] PID %-6s | %-45s | %s\n",
               cls, entry->d_name, ctx, cmd);
    }

    closedir(proc);

    printf("\n=== Summary ===\n");
    printf("APP users    : %d\n", app);
    printf("SYSTEM users : %d\n", system);
    printf("VENDOR users : %d\n", vendor);
    printf("OTHER users  : %d\n", other);
    printf("TOTAL        : %d\n", app + system + vendor + other);

    if (app > 0) {
        printf("\nObservation:\n");
        printf("Applications currently hold DMA-BUF descriptors.\n");
        printf("Cross-process graphics delegation is active.\n");
    }

    return 0;
}
```
