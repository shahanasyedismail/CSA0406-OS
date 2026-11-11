#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#define MAXP 1000
#define MAXR 8

int maxm[MAXP][MAXR], alloc[MAXP][MAXR], avail[MAXR];

int is_safe(int n, int m, int seq[])
{
    int work[MAXR], finish[MAXP] = {0}, count = 0;
    memcpy(work, avail, sizeof(int) * m);
    while (count < n) {
        int found = 0;
        for (int p = 0; p < n; p++) {
            if (finish[p]) continue;
            int ok = 1;
            for (int r = 0; r < m; r++)
                if (maxm[p][r] - alloc[p][r] > work[r]) { ok = 0; break; }
            if (ok) {
                for (int r = 0; r < m; r++) work[r] += alloc[p][r];
                finish[p] = 1;
                seq[count++] = p;
                found = 1;
            }
        }
        if (!found) return 0;
    }
    return 1;
}

/* Worst case: only one process can finish per pass (the last one first,
   then the one before it, and so on), so the while loop runs n times. */
void build(int n, int m)
{
    memset(avail, 0, sizeof avail);
    for (int i = 0; i < n; i++)
        for (int r = 0; r < m; r++) {
            alloc[i][r] = 1;
            maxm[i][r] = 1;                 /* need = 0 */
        }
    for (int i = 0; i < n; i++)
        maxm[i][m - 1] = alloc[i][m - 1] + (n - i);  /* need = n - i */
    avail[m - 1] = 1;
}

double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

void measure(int n, int m)
{
    static int seq[MAXP];
    build(n, m);
    int reps = 2000000 / (n * n) + 1;
    int safe = 0;
    double t0 = now();
    for (int k = 0; k < reps; k++) safe = is_safe(n, m, seq);
    double per = (now() - t0) / reps;
    printf("n=%4d m=%d  safe=%d  time=%10.2f us  time/n^2=%6.2f ns\n",
           n, m, safe, per * 1e6, per * 1e9 / ((double)n * n));
}

int main(void)
{
    printf("Growth with n (m = 3):\n");
    measure(10, 3);
    measure(100, 3);
    measure(1000, 3);
    printf("\nGrowth with m (n = 1000):\n");
    measure(1000, 2);
    measure(1000, 4);
    measure(1000, 8);
    return 0;
}
