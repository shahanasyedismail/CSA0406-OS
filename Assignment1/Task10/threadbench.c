#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>

static void *work(void *arg) { (void)arg; return NULL; }

int main(void)
{
    pthread_t t;
    for (int i = 0; i < 1000; i++) {
        if (pthread_create(&t, NULL, work, NULL) != 0) { perror("pthread_create"); return 1; }
        pthread_join(t, NULL);
    }
    printf("created 1000 threads\n");
    return 0;
}
