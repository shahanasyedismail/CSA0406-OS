#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define OPS 1000000

volatile long unsafe_count = 0;
long safe_count = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *unsafe_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < OPS; i++) unsafe_count++;   /* read, add, write: not atomic */
    return NULL;
}

void *safe_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < OPS; i++) {
        pthread_mutex_lock(&m);
        safe_count++;
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int n = argc > 1 ? atoi(argv[1]) : 4;
    pthread_t th[n];
    for (int i = 0; i < n; i++) pthread_create(&th[i], NULL, unsafe_worker, NULL);
    for (int i = 0; i < n; i++) pthread_join(th[i], NULL);
    printf("no protection: %ld (expected %ld)\n", unsafe_count, (long)n * OPS);

    for (int i = 0; i < n; i++) pthread_create(&th[i], NULL, safe_worker, NULL);
    for (int i = 0; i < n; i++) pthread_join(th[i], NULL);
    printf("with mutex:    %ld (expected %ld)\n", safe_count, (long)n * OPS);
    return 0;
}
