/* detect.c - deadlock detection + recovery timing (Q6.F)
 * Compile: gcc -std=c11 -Wall -Wextra -O2 detect.c -o detect
 * Run:     ./detect
 *
 * Scenario: P0 holds R0 and wants R1; P1 holds R1 and wants R0 (deadlock).
 * Processes P2..P(n-1) form a chain that needs n-1 passes, so the
 * detection cost grows like O(m * n^2), the same shape as the safety check.
 * Recovery: abort the deadlocked process with the least progress (lowest cost),
 * release its resources and run detection again to confirm the deadlock is gone.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M 3

static double now_us(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

/* returns number of deadlocked processes; marks them in dead[] */
static int detect(int n, int (*alloc)[M], int (*req)[M], const int *avail, char *dead) {
    int work[M];
    char *finish = calloc(n, 1);
    memcpy(work, avail, sizeof work);
    for (int p = 0; p < n; p++) {
        int zero = 1;
        for (int r = 0; r < M; r++) if (alloc[p][r]) zero = 0;
        if (zero) finish[p] = 1;
    }
    for (;;) {
        int found = 0;
        for (int p = 0; p < n; p++) {
            if (finish[p]) continue;
            int ok = 1;
            for (int r = 0; r < M; r++) if (req[p][r] > work[r]) { ok = 0; break; }
            if (ok) {
                for (int r = 0; r < M; r++) work[r] += alloc[p][r];
                finish[p] = 1;
                found = 1;
            }
        }
        if (!found) break;
    }
    int cnt = 0;
    for (int p = 0; p < n; p++) { dead[p] = !finish[p]; cnt += dead[p]; }
    free(finish);
    return cnt;
}

int main(void) {
    int sizes[3] = {10, 100, 1000};
    int reps[3]  = {20000, 500, 10};
    int progress[2] = {40, 25};            /* assumed units of work done by P0 and P1 (out of 100) */

    printf("Deadlock detection and recovery (m = %d)\n", M);
    for (int s = 0; s < 3; s++) {
        int n = sizes[s];
        int (*alloc)[M] = calloc(n, sizeof *alloc);
        int (*req)[M]   = calloc(n, sizeof *req);
        char *dead      = calloc(n, 1);
        int avail[M]    = {0, 0, 1};

        alloc[0][0] = 1; req[0][1] = 1;    /* P0 holds R0, wants R1 */
        alloc[1][1] = 1; req[1][0] = 1;    /* P1 holds R1, wants R0 */
        for (int i = 2; i < n; i++) { alloc[i][2] = 1; req[i][2] = n - i; }

        int cnt = detect(n, alloc, req, avail, dead);   /* warm-up + result */
        double t0 = now_us();
        for (int k = 0; k < reps[s]; k++) detect(n, alloc, req, avail, dead);
        double det = (now_us() - t0) / reps[s];

        /* recovery: abort the deadlocked process with least progress */
        int victim = progress[1] < progress[0] ? 1 : 0;
        double r0 = now_us();
        for (int r = 0; r < M; r++) { avail[r] += alloc[victim][r]; alloc[victim][r] = 0; req[victim][r] = 0; }
        int after = detect(n, alloc, req, avail, dead);
        double rec = now_us() - r0;

        printf("n=%4d  deadlocked=%d  detection=%10.2f us (avg of %d runs)  "
               "abort P%d: work lost=%d of 100 units, recovery+recheck=%.2f us, deadlocked after=%d\n",
               n, cnt, det, reps[s], victim, progress[victim], rec, after);
        free(alloc); free(req); free(dead);
    }
    return 0;
}
