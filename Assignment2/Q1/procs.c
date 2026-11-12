#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#define KIDS 4

static void show(pid_t pid, const char *role)
{
    char path[64], line[256];
    snprintf(path, sizeof path, "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) { printf("%d: no /proc entry\n", pid); return; }
    printf("--- child %d (%s)\n", pid, role);
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, "State", 5) || !strncmp(line, "PPid", 4) ||
            !strncmp(line, "VmRSS", 5) || !strncmp(line, "voluntary_ctxt", 14) ||
            !strncmp(line, "nonvoluntary_ctxt", 17))
            fputs(line, stdout);
    fclose(f);
}

int main(void)
{
    const char *role[KIDS] = {"busy loop (running/ready)", "sleeping (waiting)",
                              "sleeping (waiting)", "exits at once (zombie)"};
    pid_t pid[KIDS];
    printf("parent pid = %d\n", getpid());
    for (int i = 0; i < KIDS; i++) {
        pid[i] = fork();
        if (pid[i] < 0) { perror("fork"); return 1; }
        if (pid[i] == 0) {
            if (i == 0) { time_t end = time(NULL) + 25; while (time(NULL) < end) ; }
            else if (i < 3) sleep(25);
            _exit(0);
        }
    }
    sleep(2);
    for (int i = 0; i < KIDS; i++) show(pid[i], role[i]);
    printf("\nNow run ps in a second terminal. Waiting 20 s...\n");
    fflush(stdout);
    sleep(20);
    for (int i = 0; i < KIDS; i++) waitpid(pid[i], NULL, 0);
    printf("all children reaped\n");
    return 0;
}
