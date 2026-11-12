#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#define CAMERAS 64
#define WORK 10000000L
#define SPAWN 2000

typedef struct { atomic_int next; unsigned long result[CAMERAS]; } shared_t;
static shared_t *sh;

typedef struct { int cam; unsigned long h; } rec_t;

static unsigned long process_frame(int cam)
{
    unsigned long h = 1469598103934665603UL ^ (unsigned long)cam;
    for (long i = 0; i < WORK; i++) {
        h ^= (unsigned long)i;
        h *= 1099511628211UL;
    }
    return h;
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static unsigned long checksum(void)
{
    unsigned long s = 0;
    for (int i = 0; i < CAMERAS; i++) s ^= sh->result[i] + (unsigned long)i;
    return s;
}

static void reset(void)
{
    atomic_store(&sh->next, 0);
    memset(sh->result, 0, sizeof sh->result);
}

/* mode 0 sequential, 1 process per camera (shared memory),
   2 pool of n processes (shared memory), 3 pool of n processes (pipe) */
static double run(int mode, int n)
{
    reset();
    int fd[2];
    if (mode == 3 && pipe(fd) == -1) { perror("pipe"); exit(1); }
    double t0 = now();

    if (mode == 0) {
        for (int c = 0; c < CAMERAS; c++) sh->result[c] = process_frame(c);
        return now() - t0;
    }
    int kids = mode == 1 ? CAMERAS : n;
    for (int i = 0; i < kids; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            if (mode == 1) {
                sh->result[i] = process_frame(i);
            } else if (mode == 2) {
                int c;
                while ((c = atomic_fetch_add(&sh->next, 1)) < CAMERAS)
                    sh->result[c] = process_frame(c);
            } else {
                close(fd[0]);
                for (int c = i; c < CAMERAS; c += n) {
                    rec_t r = {c, process_frame(c)};
                    if (write(fd[1], &r, sizeof r) < 0) perror("write");
                }
                close(fd[1]);
            }
            _exit(0);
        }
    }
    if (mode == 3) {
        close(fd[1]);
        rec_t r;
        while (read(fd[0], &r, sizeof r) == (ssize_t)sizeof r)
            sh->result[r.cam] = r.h;
        close(fd[0]);
    }
    for (int i = 0; i < kids; i++) wait(NULL);
    return now() - t0;
}

static void *empty_thread(void *a) { return a; }

static void spawn_cost(void)
{
    double t0 = now();
    for (int i = 0; i < SPAWN; i++) {
        pthread_t t;
        pthread_create(&t, NULL, empty_thread, NULL);
        pthread_join(t, NULL);
    }
    double tt = (now() - t0) / SPAWN;
    t0 = now();
    for (int i = 0; i < SPAWN; i++) {
        pid_t pid = fork();
        if (pid == 0) _exit(0);
        waitpid(pid, NULL, 0);
    }
    double tp = (now() - t0) / SPAWN;
    printf("create + finish one thread : %7.1f us\n", tt * 1e6);
    printf("create + finish one process: %7.1f us  (%.1fx slower)\n\n",
           tp * 1e6, tp / tt);
}

int main(void)
{
    sh = mmap(NULL, sizeof *sh, PROT_READ | PROT_WRITE,
              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sh == MAP_FAILED) { perror("mmap"); return 1; }

    printf("cores online: %ld\n\n", sysconf(_SC_NPROCESSORS_ONLN));
    spawn_cost();

    struct { const char *name; int mode, n; } cfg[] = {
        {"sequential", 0, 1},
        {"4 procs, shared mem", 2, 4},
        {"8 procs, shared mem", 2, 8},
        {"16 procs, shared mem", 2, 16},
        {"4 procs, pipe", 3, 4},
        {"8 procs, pipe", 3, 8},
        {"process per camera", 1, CAMERAS},
    };
    printf("%-22s %9s %9s %s\n", "method", "time (s)", "speed-up", "check");
    double base = 0;
    unsigned long ref = 0;
    for (int k = 0; k < 7; k++) {
        double best = 1e9;
        for (int rep = 0; rep < 3; rep++) {
            double t = run(cfg[k].mode, cfg[k].n);
            if (t < best) best = t;
        }
        unsigned long cs = checksum();
        if (k == 0) { base = best; ref = cs; }
        printf("%-22s %9.3f %8.2fx %s\n", cfg[k].name, best, base / best,
               cs == ref ? "OK" : "WRONG");
    }
    munmap(sh, sizeof *sh);
    return 0;
}
