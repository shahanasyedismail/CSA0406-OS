# CSA04 Assignment 1 - Operating Systems for a Smart Hospital

Name: SHAHANA
Register Number: 192524479
Compiler: gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0
Linux: Ubuntu 26.04.1 LTS (WSL on Windows)

## Compile command
gcc -std=c11 -Wall -Wextra -pthread <file>.c -o <program>

## Folders
- Task06: task6.c, screenshots (output, strace, time)
- Task07: task7.c, task7b.c, out.txt, screenshots
- Task10: forkbench.c, threadbench.c, screenshots

## How to run
- Task06: gcc -std=c11 -Wall -Wextra -pthread task6.c -o task6 && ./task6
- Task07: gcc -std=c11 -Wall -Wextra -pthread task7.c -o task7 && ./task7
- Task07 experiment: compile task7b.c, run ./task7b > out.txt, then cat out.txt
- Task10: compile forkbench.c and threadbench.c, run each with time

## Results
- Task 6: read() calls 10001 / 21 / 4 for buffers 1 / 512 / 4096 (matches expected). strace -c shows 10027 read calls in total (loader adds 1). time: real 0.038s, user 0.000s, sys 0.009s.
- Task 7: output matches expected. With exit(3), no second fflush and output redirected to a file, buffered lines are printed twice.
- Task 10: average of 3 runs, 1000 processes 0.321s real, 1000 threads 0.093s real. Threads are about 3.5x faster.
- Tasks 1-5, 8, 9, 11, 12: analysis only, see the report.
