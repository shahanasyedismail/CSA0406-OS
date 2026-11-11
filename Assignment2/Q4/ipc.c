#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#define CHUNK 65536
#define SLOTS 16
#define MB (1024UL * 1024UL)

typedef struct {
    sem_t empty, full;
    unsigned char slot[SLOTS][CHUNK];
} shm_t;

static unsigned char src[CHUNK], dst[CHUNK];

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static long checksum(const unsigned char *b, size_t n)
{
    long s = 0;
    for (size_t i = 0; i < n; i++) s += b[i];
    return s;
}

/* returns elapsed seconds, or -1 if the data was corrupted */
static double run_pipe(size_t total)
{
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); exit(1); }
    double t0 = now();
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]);
        long s = 0;
        size_t got = 0;
        ssize_t n;
        while ((n = read(fd[0], dst, CHUNK)) > 0) {
            s += checksum(dst, (size_t)n);
            got += (size_t)n;
        }
        _exit(got == total && s == 7L * (long)total ? 0 : 1);
    }
    close(fd[0]);
    for (size_t sent = 0; sent < total; sent += CHUNK)
        if (write(fd[1], src, CHUNK) < 0) perror("write");
    close(fd[1]);
    int st;
    waitpid(pid, &st, 0);
    double el = now() - t0;
    return WEXITSTATUS(st) == 0 ? el : -1;
}

static double run_shm(size_t total)
{
    shm_t *sh = mmap(NULL, sizeof *sh, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sh == MAP_FAILED) { perror("mmap"); exit(1); }
    sem_init(&sh->empty, 1, SLOTS);   /* pshared = 1: shared between processes */
    sem_init(&sh->full, 1, 0);
    size_t chunks = total / CHUNK;
    double t0 = now();
    pid_t pid = fork();
    if (pid == 0) {
        long s = 0;
        for (size_t k = 0; k < chunks; k++) {
            sem_wait(&sh->full);
            s += checksum(sh->slot[k % SLOTS], CHUNK);
            sem_post(&sh->empty);
        }
        _exit(s == 7L * (long)total ? 0 : 1);
    }
    for (size_t k = 0; k < chunks; k++) {
        sem_wait(&sh->empty);
        memcpy(sh->slot[k % SLOTS], src, CHUNK);
        sem_post(&sh->full);
    }
    int st;
    waitpid(pid, &st, 0);
    double el = now() - t0;
    sem_destroy(&sh->empty);
    sem_destroy(&sh->full);
    munmap(sh, sizeof *sh);
    return WEXITSTATUS(st) == 0 ? el : -1;
}

int main(void)
{
    memset(src, 7, sizeof src);
    size_t sizes[] = {1, 10, 100};
    printf("%-8s %-14s %-10s %-10s\n", "size", "method", "time (s)", "MB/s");
    for (int i = 0; i < 3; i++) {
        size_t total = sizes[i] * MB;
        for (int m = 0; m < 2; m++) {
            double best = 1e9;
            for (int rep = 0; rep < 3; rep++) {
                double t = m == 0 ? run_pipe(total) : run_shm(total);
                if (t < 0) { printf("DATA CORRUPTED\n"); return 1; }
                if (t < best) best = t;
            }
            printf("%4zu MB  %-14s %-10.4f %-10.0f\n", sizes[i],
                   m == 0 ? "pipe" : "shared memory", best, sizes[i] / best);
        }
    }
    return 0;
}
