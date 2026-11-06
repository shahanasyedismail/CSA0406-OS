#!/bin/bash
# Run inside the folder that contains task6.c, forkbench.c and threadbench.c (WSL/Ubuntu).
# Times each buffer size separately by repeating the copy, and repeats the fork/thread benchmarks 10 times.
set -e
gcc -std=c11 -Wall -Wextra -pthread forkbench.c -o forkbench
gcc -std=c11 -Wall -Wextra -pthread threadbench.c -o threadbench

# --- Part 1: per-buffer-size copy timing (needs bench6.c, written below) ---
cat > bench6.c <<'EOC'
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
static long copy_file(const char *src,const char *dst,size_t bufsize){
    char buf[8192]; if(bufsize>sizeof buf) return -1;
    int in=open(src,O_RDONLY); if(in<0) return -1;
    int out=open(dst,O_WRONLY|O_CREAT|O_TRUNC,0644); if(out<0){close(in);return -1;}
    long calls=0; ssize_t n;
    for(;;){ n=read(in,buf,bufsize); calls++; if(n<=0)break; if(write(out,buf,n)!=n){calls=-1;break;} }
    close(in); close(out); return calls;
}
int main(void){
    FILE *f=fopen("records.dat","w"); for(int i=0;i<10000;i++) fputc('A'+i%26,f); fclose(f);
    size_t sizes[3]={1,512,4096}; int reps=1000;
    for(int s=0;s<3;s++){
        struct timespec a,b; long calls=0;
        clock_gettime(CLOCK_MONOTONIC,&a);
        for(int r=0;r<reps;r++) calls=copy_file("records.dat","copy.dat",sizes[s]);
        clock_gettime(CLOCK_MONOTONIC,&b);
        double ms=((b.tv_sec-a.tv_sec)*1e3+(b.tv_nsec-a.tv_nsec)/1e6);
        printf("buffer %5zu B: read() calls per copy=%ld, total %.1f ms for %d copies, %.3f ms per copy\n",sizes[s],calls,ms,reps,ms/reps);
    }
    return 0;
}
EOC
gcc -std=c11 -Wall -Wextra bench6.c -o bench6
echo "=== Per-buffer copy timing (run 3 times) ==="
for i in 1 2 3; do ./bench6; echo; done

# --- Part 2: 10 runs each of the process/thread benchmarks ---
echo "=== forkbench x10 (real/user/sys) ==="
for i in $(seq 1 10); do /usr/bin/time -f "run $i: real %e user %U sys %S" ./forkbench >/dev/null; done
echo "=== threadbench x10 (real/user/sys) ==="
for i in $(seq 1 10); do /usr/bin/time -f "run $i: real %e user %U sys %S" ./threadbench >/dev/null; done
