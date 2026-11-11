#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

int nthreads, ops, work_iters;
atomic_long a_counter;
long counter;
atomic_flag spin = ATOMIC_FLAG_INIT;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int mode;

static void work(void)
{
    volatile int x = 0;
    for (int i = 0; i < work_iters; i++) x += i;
}

void *worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ops; k++) {
        if (mode == 0) {
            atomic_fetch_add(&a_counter, 1);
        } else if (mode == 1) {
            while (atomic_flag_test_and_set_explicit(&spin, memory_order_acquire))
                ;                       /* spin (busy wait) */
            counter++;
            work();
            atomic_flag_clear_explicit(&spin, memory_order_release);
        } else {
            pthread_mutex_lock(&mtx);
            counter++;
            work();
            pthread_mutex_unlock(&mtx);
        }
    }
    return NULL;
}

double clk(clockid_t id)
{
    struct timespec ts;
    clock_gettime(id, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char *argv[])
{
    nthreads = argc > 1 ? atoi(argv[1]) : 4;
    ops = argc > 2 ? atoi(argv[2]) : 1000000;
    work_iters = argc > 3 ? atoi(argv[3]) : 0;
    const char *names[] = {"atomic_fetch_add", "spin lock", "mutex"};

    for (mode = 0; mode < 3; mode++) {
        if (mode == 0 && work_iters > 0) continue;  /* atomic cannot cover a long section */
        counter = 0;
        atomic_store(&a_counter, 0);
        pthread_t th[nthreads];
        double w0 = clk(CLOCK_MONOTONIC), c0 = clk(CLOCK_PROCESS_CPUTIME_ID);
        for (int i = 0; i < nthreads; i++) pthread_create(&th[i], NULL, worker, NULL);
        for (int i = 0; i < nthreads; i++) pthread_join(th[i], NULL);
        double wall = clk(CLOCK_MONOTONIC) - w0;
        double cpu = clk(CLOCK_PROCESS_CPUTIME_ID) - c0;
        long total = mode == 0 ? atomic_load(&a_counter) : counter;
        long expected = (long)nthreads * ops;
        printf("%-17s threads=%2d wall=%7.3f s cpu=%7.3f s  %7.1f ns/update  %s\n",
               names[mode], nthreads, wall, cpu, wall * 1e9 / expected,
               total == expected ? "count OK" : "WRONG");
    }
    return 0;
}
