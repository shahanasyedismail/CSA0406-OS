#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    for (int i = 0; i < 1000; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) _exit(0);
        waitpid(pid, NULL, 0);
    }
    printf("created 1000 processes\n");
    return 0;
}
