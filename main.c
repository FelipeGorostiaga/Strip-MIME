#define BLOCK 100
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char * argv []) {
    char * command = NULL;
    char commandOutput[1000] = {0};
    char stdInput[BLOCK] = {0};
    size_t cmdLength = 0;
    int i, j;
    int fds[2] = {-1,-1};
    ssize_t bytesRead;
    int status;

    if(argc <= 1)
        return 0;

    for(i = 1; i < argc; i++) {
        for(j = 0; argv[i][j] != 0; j++) {
            if(cmdLength % BLOCK == 0 ) {
                command = realloc(command,cmdLength + BLOCK);
            }
            command[cmdLength++] = argv[i][j];
        }
    }
    if(cmdLength % BLOCK == 0) {
        command = realloc(command,cmdLength + BLOCK);
    }

    if(command != NULL) {
        command[cmdLength] = 0;
    } else {
        exit(1);
    }

    if(pipe(fds) < 0) { exit(1); }

    if(fork() != 0) {
        close(fds[1]);
        dup2(fds[0], 0);
        int status;
        while(wait(&status) > 0);
        scanf("%s", stdInput); // despues de que hijo haya escrito en stdin de padre
        printf("%s\n", stdInput);
    }
    else {
        close(fds[0]);
        dup2(fds[1], 1);
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
    }
}