#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#define SIZE 5
#define ITEMS 20
#ifdef NOLOCK
#define LOCK()
#define UNLOCK()
#else
#define LOCK() pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)
#endif

int buffer[SIZE], in = 0, out = 0;
sem_t empty, full;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
long consumed_sum = 0;
int counts[ITEMS + 1];
int per_consumer;

void *producer(void *arg)
{
    (void)arg;
    for (int item = 1; item <= ITEMS; item++) {
        sem_wait(&empty);
        LOCK();
        buffer[in] = item;
        in = (in + 1) % SIZE;
        UNLOCK();
        sem_post(&full);
    }
    return NULL;
}

void *consumer(void *arg)
{
    (void)arg;
    for (int k = 0; k < per_consumer; k++) {
        sem_wait(&full);
        LOCK();
        int item = buffer[out];
        out = (out + 1) % SIZE;
        consumed_sum += item;
        if (item >= 1 && item <= ITEMS) counts[item]++;
        UNLOCK();
        sem_post(&empty);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int np = argc > 1 ? atoi(argv[1]) : 4;
    int nc = argc > 2 ? atoi(argv[2]) : 4;
    if (np < 1 || nc < 1 || (np * ITEMS) % nc != 0) {
        fprintf(stderr, "need (producers * %d) divisible by consumers\n", ITEMS);
        return 1;
    }
    per_consumer = np * ITEMS / nc;
    sem_init(&empty, 0, SIZE);
    sem_init(&full, 0, 0);
    pthread_t p[np], c[nc];
    for (int i = 0; i < np; i++) pthread_create(&p[i], NULL, producer, NULL);
    for (int i = 0; i < nc; i++) pthread_create(&c[i], NULL, consumer, NULL);
    for (int i = 0; i < np; i++) pthread_join(p[i], NULL);
    for (int i = 0; i < nc; i++) pthread_join(c[i], NULL);

    long expected = (long)np * ITEMS * (ITEMS + 1) / 2;
    int bad = 0;
    for (int v = 1; v <= ITEMS; v++) if (counts[v] != np) bad++;
    printf("P=%d C=%d: consumed %d items, sum = %ld (expected %ld), "
           "values not consumed exactly %d times: %d -> %s\n",
           np, nc, np * ITEMS, consumed_sum, expected, np, bad,
           (consumed_sum == expected && bad == 0) ? "OK" : "WRONG");
    return 0;
}
