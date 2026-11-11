#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

static long copy_file(const char *src, const char *dst, size_t bufsize)
{
    char buf[8192];
    if (bufsize > sizeof buf) return -1;
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
    FILE *f = fopen("records.dat", "w");
    for (int i = 0; i < 10000; i++) fputc('A' + i % 26, f);
    fclose(f);
    size_t sizes[3] = {1, 512, 4096}; int reps = 1000;
    for (int s = 0; s < 3; s++) {
        struct timespec a, b; long calls = 0;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int r = 0; r < reps; r++) calls = copy_file("records.dat", "copy.dat", sizes[s]);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms = (b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6;
        printf("buffer %5zu B: read() calls per copy=%ld, total %.1f ms for %d copies, %.3f ms per copy\n",
               sizes[s], calls, ms, reps, ms / reps);
    }
    return 0;
}
