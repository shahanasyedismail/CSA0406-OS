#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#define TRAINS 10
#define COACHES 10
#define WORK 1000   /* simulated work inside the critical section */

int mode, nthreads, ops;
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t train_lock[TRAINS];
pthread_mutex_t coach_lock[TRAINS][COACHES];
long booked[TRAINS][COACHES];

static void work(void)
{
    volatile int x = 0;
    for (int i = 0; i < WORK; i++) x += i;
}

void *agent(void *arg)
{
    unsigned seed = (unsigned)(long)arg + 1;
    for (int k = 0; k < ops; k++) {
        int t = rand_r(&seed) % TRAINS;
        int c = rand_r(&seed) % COACHES;
        pthread_mutex_t *m = mode == 0 ? &global_lock
                           : mode == 1 ? &train_lock[t]
                                       : &coach_lock[t][c];
        pthread_mutex_lock(m);
        booked[t][c]++;      /* book one seat */
        work();
        pthread_mutex_unlock(m);
    }
    return NULL;
}

double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char *argv[])
{
    nthreads = argc > 1 ? atoi(argv[1]) : 8;
    ops = argc > 2 ? atoi(argv[2]) : 100000;
    const char *names[] = {"one global lock", "lock per train", "lock per coach"};

    for (int t = 0; t < TRAINS; t++) {
        pthread_mutex_init(&train_lock[t], NULL);
        for (int c = 0; c < COACHES; c++) pthread_mutex_init(&coach_lock[t][c], NULL);
    }
    for (mode = 0; mode < 3; mode++) {
        for (int t = 0; t < TRAINS; t++)
            for (int c = 0; c < COACHES; c++) booked[t][c] = 0;
        pthread_t th[nthreads];
        double t0 = now();
        for (long i = 0; i < nthreads; i++) pthread_create(&th[i], NULL, agent, (void *)i);
        for (int i = 0; i < nthreads; i++) pthread_join(th[i], NULL);
        double el = now() - t0;
        long total = 0;
        for (int t = 0; t < TRAINS; t++)
            for (int c = 0; c < COACHES; c++) total += booked[t][c];
        printf("%-16s threads=%2d  time=%6.3f s  throughput=%9.0f bookings/s  %s\n",
               names[mode], nthreads, el, total / el,
               total == (long)nthreads * ops ? "count OK" : "LOST UPDATES");
    }
    return 0;
}
