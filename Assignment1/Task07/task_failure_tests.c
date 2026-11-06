#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

/* Same copy loop as task6.c, with the bufsize guard from bench6.c */
static long copy_file(const char *src, const char *dst, size_t bufsize)
{
    char buf[8192];
    if (bufsize == 0 || bufsize > sizeof buf) return -1;
    int in = open(src, O_RDONLY); if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); return -1; }
    long calls = 0; ssize_t n;
    for (;;) {
        n = read(in, buf, bufsize); calls++;
        if (n <= 0) break;
        if (write(out, buf, n) != n) { calls = -1; break; }
    }
    close(in); close(out); return calls;
}

int main(void)
{
    /* Task 6 boundary tests */
    printf("missing source file   -> %ld (expected -1)\n", copy_file("no_such_file.dat", "x.dat", 512));
    FILE *f = fopen("empty.dat", "w"); fclose(f);
    printf("empty file, 512 bytes -> %ld (expected 1: only the EOF read)\n", copy_file("empty.dat", "x.dat", 512));
    printf("bufsize 9000 (> 8192) -> %ld (expected -1)\n", copy_file("empty.dat", "x.dat", 9000));
    unlink("empty.dat"); unlink("x.dat");

    /* Task 7 failure test: exec of a command that does not exist */
    pid_t pid = fork();
    if (pid == 0) {
        execlp("no_such_command", "no_such_command", (char *)NULL);
        _exit(127);
    }
    int status; waitpid(pid, &status, 0);
    printf("exec of missing command -> exit status %d (expected 127)\n", WEXITSTATUS(status));

    /* Task 7 zombie demonstration: child exits, parent does NOT wait for 20 s */
    pid = fork();
    if (pid == 0) _exit(0);
    printf("child %d has exited; run 'ps -o pid,stat,cmd --ppid %d' in another terminal now\n", pid, getpid());
    fflush(stdout);
    sleep(20);
    waitpid(pid, NULL, 0);
    printf("zombie reaped\n");
    return 0;
}
