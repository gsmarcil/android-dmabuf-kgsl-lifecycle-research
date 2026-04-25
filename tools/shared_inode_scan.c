#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

/*
 * shared_inode_scan.c
   * Finds DMA-BUF inodes shared across different SELinux domains.
   */

#define MAX_RECORDS 16384
#define MAX_SEEN    4096

typedef struct {
    char inode[64];
    pid_t pid;
    char ctx[128];
    char cmd[256];
} Record;

static void read_cmdline(pid_t pid, char *buf, size_t buflen) {
      if (buflen == 0) return;
    snprintf(buf, buflen, "unknown");
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    FILE *f = fopen(path, "r");
    if (!f) return;
    size_t n = fread(buf, 1, buflen - 1, f);
    fclose(f);
    if (n == 0) { snprintf(buf, buflen, "unknown"); return; }
    buf[n] = '\0';
    for (size_t i = 0; i < n; i++) { if (buf[i] == '\0') buf[i] = ' '; }
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
    } else { snprintf(buf, buflen, "unknown"); }
    fclose(f);
}

static int collect_records(Record *recs, int max_records) {
      int count = 0;
    DIR *proc = opendir("/proc");
    if (!proc) return 0;
    struct dirent *entry;
    while ((entry = readdir(proc))) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
        pid_t pid = (pid_t)atoi(entry->d_name);
        char ctx[128], cmd[256];
        read_context(pid, ctx, sizeof(ctx));
        read_cmdline(pid, cmd, sizeof(cmd));
        char fd_dir[PATH_MAX];
        snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", entry->d_name);
        DIR *fd_d = opendir(fd_dir);
        if (!fd_d) continue;
        struct dirent *fe;
        while ((fe = readdir(fd_d))) {
            if (fe->d_name[0] == '.') continue;
            if (count >= max_records) break;
            char link_path[PATH_MAX], target[PATH_MAX];
            snprintf(link_path, sizeof(link_path), "/proc/%s/fd/%s", entry->d_name, fe->d_name);
            ssize_t len = readlink(link_path, target, sizeof(target) - 1);
            if (len <= 0) continue;
            target[len] = '\0';
            if (strstr(target, "dmabuf")) {
                char inode[64] = {0};
                if (sscanf(target, "/dmabuf:%63s", inode) == 1 && inode[0]) {
                    snprintf(recs[count].inode, sizeof(recs[count].inode), "%s", inode);
                    recs[count].pid = pid;
                    snprintf(recs[count].ctx, sizeof(recs[count].ctx), "%s", ctx);
                    snprintf(recs[count].cmd, sizeof(recs[count].cmd), "%s", cmd);
                    count++;
                }
            }
        }
        closedir(fd_d);
    }
    closedir(proc);
    return count;
}

static int inode_has_multi_domain(const Record *recs, int n, const char *inode) {
      char first_ctx[128] = {0};
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(recs[i].inode, inode) != 0) continue;
        if (!found) {
            snprintf(first_ctx, sizeof(first_ctx), "%s", recs[i].ctx);
            found = 1;
        } else if (strcmp(first_ctx, recs[i].ctx) != 0) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
      printf("=== Shared DMA-BUF Inode Scanner ===\n");
    printf("Searching for cross-domain shared inodes...\n\n");
    static Record records[MAX_RECORDS];
    int n = collect_records(records, MAX_RECORDS);
    if (n <= 0) { printf("No DMA-BUF records.\n"); return 0; }
    char seen[MAX_SEEN][64];
    int seen_count = 0;
    int found_any = 0;
    for (int i = 0; i < n; i++) {
        if (seen_count > 0 && strcmp(seen[0], records[i].inode) == 0) continue;
        if (seen_count < MAX_SEEN) {
            snprintf(seen[seen_count], sizeof(seen[seen_count]), "%s", records[i].inode);
            seen_count++;
        }
        if (inode_has_multi_domain(records, n, records[i].inode)) {
            printf("%s shared by:\n", records[i].inode);
            for (int j = 0; j < n; j++) {
                if (strcmp(records[j].inode, records[i].inode) == 0) {
                    printf("  PID %-6d | %-45s | %s\n", records[j].pid, records[j].ctx, records[j].cmd);
                }
            }
            printf("\n");
            found_any = 1;
        }
    }
    if (!found_any) { printf("No cross-domain shared inodes found.\n"); }
    return 0;
}
