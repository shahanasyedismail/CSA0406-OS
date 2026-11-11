#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>
#define CAMERAS 64
#define WORK 10000000L

static unsigned long result[CAMERAS];
static atomic_int next_cam;

static unsigned long process_frame(int cam)
{
    unsigned long h = 1469598103934665603UL ^ (unsigned long)cam;
    for (long i = 0; i < WORK; i++) {
        h ^= (unsigned long)i;
        h *= 1099511628211UL;
    }
    return h;
}

static void *cam_thread(void *arg)          /* one thread = one camera */
{
    int c = (int)(long)arg;
    result[c] = process_frame(c);
    return NULL;
}

static void *pool_thread(void *arg)         /* worker takes the next camera */
{
    (void)arg;
    int c;
    while ((c = atomic_fetch_add(&next_cam, 1)) < CAMERAS)
        result[c] = process_frame(c);
    return NULL;
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
    for (int i = 0; i < CAMERAS; i++) s ^= result[i] + (unsigned long)i;
    return s;
}

/* mode 0 = sequential, 1 = thread per camera, 2 = pool of n threads */
static double run(int mode, int n)
{
    atomic_store(&next_cam, 0);
    for (int i = 0; i < CAMERAS; i++) result[i] = 0;
    double t0 = now();
    if (mode == 0) {
        for (int c = 0; c < CAMERAS; c++) result[c] = process_frame(c);
    } else {
        pthread_t th[CAMERAS];
        for (int i = 0; i < n; i++)
            pthread_create(&th[i], NULL, mode == 1 ? cam_thread : pool_thread,
                           (void *)(long)i);
        for (int i = 0; i < n; i++) pthread_join(th[i], NULL);
    }
    return now() - t0;
}

int main(void)
{
    struct { const char *name; int mode, n; } cfg[] = {
        {"sequential", 0, 1},
        {"pool of 4", 2, 4},
        {"pool of 8", 2, 8},
        {"pool of 16", 2, 16},
        {"thread per camera", 1, CAMERAS},
    };
    printf("cores online: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("%-18s %9s %9s %s\n", "method", "time (s)", "speed-up", "check");
    double base = 0;
    unsigned long ref = 0;
    for (int k = 0; k < 5; k++) {
        double best = 1e9;
        for (int rep = 0; rep < 3; rep++) {
            double t = run(cfg[k].mode, cfg[k].n);
            if (t < best) best = t;
        }
        unsigned long cs = checksum();
        if (k == 0) { base = best; ref = cs; }
        printf("%-18s %9.3f %8.2fx %s\n", cfg[k].name, best, base / best,
               cs == ref ? "OK" : "WRONG");
    }
    return 0;
}
