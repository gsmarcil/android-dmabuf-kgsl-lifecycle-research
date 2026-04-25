#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>

/*
 * lifecycle_test.c
 * Observes whether SurfaceFlinger retains shared DMA-BUF inodes
 * after an application process terminates.
 *
 * This is a lifecycle / ownership analysis tool, not an exploit.
 */

typedef struct {
    pid_t pid;
    char ctx[128];
    char cmd[256];
    char inodes[2048][32];
    int inode_count;
} ProcInfo;

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

static int collect_dmabuf_inodes(pid_t pid, char inodes[][32], int max_inodes) {
    int count = 0;

    char fd_dir[PATH_MAX];
    snprintf(fd_dir, sizeof(fd_dir), "/proc/%d/fd", pid);

    DIR *d = opendir(fd_dir);
    if (!d) return 0;

    struct dirent *fe;
    while ((fe = readdir(d))) {
        if (fe->d_name[0] == '.') continue;
        if (count >= max_inodes) break;

        char link_path[PATH_MAX], target[PATH_MAX];
        snprintf(link_path, sizeof(link_path), "/proc/%d/fd/%s", pid, fe->d_name);

        ssize_t len = readlink(link_path, target, sizeof(target) - 1);
        if (len <= 0) continue;
        target[len] = '\0';

        if (strstr(target, "dmabuf")) {
            char inode[32] = {0};
            if (sscanf(target, "/dmabuf:%31s", inode) == 1 && inode[0]) {
                snprintf(inodes[count], 32, "%s", inode);
                count++;
            }
        }
    }

    closedir(d);
    return count;
}

static int inode_in_list(const char *inode, char list[][32], int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(inode, list[i]) == 0) return 1;
    }
    return 0;
}

static int count_shared(const char *base[][32], int base_n, const char *candidate[][32], int cand_n) {
    int shared = 0;
    for (int i = 0; i < base_n; i++) {
        for (int j = 0; j < cand_n; j++) {
            if (strcmp(base[i], candidate[j]) == 0) {
                shared++;
                break;
            }
        }
    }
    return shared;
}

int main(void) {
    printf("=== DMA-BUF Lifecycle Asymmetry Test ===\n\n");

    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("opendir /proc");
        return 1;
    }

    pid_t sf_pid = 0;
    ProcInfo sf = {0};

    /* Find SurfaceFlinger */
    struct dirent *entry;
    while ((entry = readdir(proc))) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;

        pid_t pid = (pid_t)atoi(entry->d_name);
        char cmd[256];
        read_cmdline(pid, cmd, sizeof(cmd));
        if (strstr(cmd, "surfaceflinger")) {
            sf_pid = pid;
            sf.pid = pid;
            snprintf(sf.cmd, sizeof(sf.cmd), "%s", cmd);
            read_context(pid, sf.ctx, sizeof(sf.ctx));
            sf.inode_count = collect_dmabuf_inodes(pid, sf.inodes, 2048);
            break;
        }
    }

    if (!sf_pid) {
        printf("SurfaceFlinger not found.\n");
        closedir(proc);
        return 1;
    }

    printf("SurfaceFlinger PID: %d\n", sf_pid);
    printf("SurfaceFlinger DMABUF count: %d\n", sf.inode_count);

    /* Find best untrusted_app candidate (highest shared count with SF) */
    pid_t target_pid = 0;
    ProcInfo target = {0};
    int best_shared = 0;

    rewinddir(proc);
    while ((entry = readdir(proc))) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;

        pid_t pid = (pid_t)atoi(entry->d_name);
        if (pid == sf_pid) continue;

        char ctx[128];
        char cmd[256];
        read_context(pid, ctx, sizeof(ctx));
        read_cmdline(pid, cmd, sizeof(cmd));

        if (!strstr(ctx, "untrusted_app")) continue;

        ProcInfo candidate = {0};
        candidate.pid = pid;
        snprintf(candidate.ctx, sizeof(candidate.ctx), "%s", ctx);
        snprintf(candidate.cmd, sizeof(candidate.cmd), "%s", cmd);
        candidate.inode_count = collect_dmabuf_inodes(pid, candidate.inodes, 2048);

        int shared = 0;
        for (int i = 0; i < candidate.inode_count; i++) {
            if (inode_in_list(candidate.inodes[i], sf.inodes, sf.inode_count)) {
                shared++;
            }
        }

        if (shared > best_shared) {
            best_shared = shared;
            target_pid = pid;
            target = candidate;
        }
    }

    if (!target_pid || best_shared < 1) {
        printf("No suitable untrusted_app with shared DMA-BUFs found.\n");
        closedir(proc);
        return 0;
    }

    printf("Target app PID: %d | shared inodes with SF: %d\n", target_pid, best_shared);
    printf("Target cmdline: %s\n", target.cmd);
    printf("Killing target PID %d...\n", target_pid);

    if (kill(target_pid, SIGKILL) != 0) {
        perror("kill");
    }
    sleep(2);

    /* Re-check SF retained inodes */
    ProcInfo sf_after = {0};
    sf_after.pid = sf_pid;
    snprintf(sf_after.ctx, sizeof(sf_after.ctx), "%s", sf.ctx);
    snprintf(sf_after.cmd, sizeof(sf_after.cmd), "%s", sf.cmd);
    sf_after.inode_count = collect_dmabuf_inodes(sf_pid, sf_after.inodes, 2048);

    int retained = 0;
    for (int i = 0; i < target.inode_count; i++) {
        if (inode_in_list(target.inodes[i], sf_after.inodes, sf_after.inode_count)) {
            retained++;
        }
    }

    printf("\n=== LIFECYCLE RESULT ===\n");
    printf("App: %s\n", target.cmd);
    printf("Shared inodes before kill: %d\n", best_shared);
    printf("SF retained after app death: %d\n", retained);

    if (retained == best_shared) {
        printf("VERDICT: SurfaceFlinger retained all shared DMA-BUFs after app death.\n");
        printf("Observation: buffers outlived the client process in this test.\n");
    } else if (retained > 0) {
        printf("VERDICT: SurfaceFlinger retained %d/%d shared DMA-BUFs.\n", retained, best_shared);
    } else {
        printf("VERDICT: SurfaceFlinger released all shared buffers upon app death.\n");
    }

    closedir(proc);
    return 0;
}
