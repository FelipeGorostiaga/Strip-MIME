#define BLOCK 100
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char * argv []) {
    char * command = NULL;
    char commandOutput[1000] = {0};
    char stdInput[BLOCK];
    size_t cmdLength = 0;
    int i, j;
    int inputFds[2] = {-1,-1};
    int outputFds[2] = {-1,-1};
    ssize_t bytesRead;
    int status;

    for(i = 1; i < argc; i++) {
        for(j = 0; argv[i][j] != 0; j++) {
            if(cmdLength % BLOCK == 0 ) {
                command = realloc(command,cmdLength + BLOCK);
            }
            command[cmdLength++] = argv[i][j];
        }
    }
    if(cmdLength % BLOCK == 0 ) {
        command = realloc(command,cmdLength + BLOCK);
    }

    if(pipe(inputFds) < 0) { exit(1); }
    if(pipe(outputFds) < 0) { exit(1); }

    if(command != NULL) {
        command[cmdLength] = 0;
        printf("\n");
    } else {
        exit(1);
    }

    scanf("%s",stdInput);
    printf("LLEGUE\n");
    write(inputFds[1],stdInput,strlen(stdInput));
    printf("ESCRIBI\n");

    if (fork() == 0) {
        close(inputFds[1]);
        close(outputFds[0]);
        dup2(inputFds[0],0);
        dup2(outputFds[1],1);
        printf("HICE DUP\n");
        execl("/bin/sh",command,NULL);
    } else {
        while(wait(&status) > 0);
        close(inputFds[0]);
        close(outputFds[1]);
        bytesRead = read(outputFds[0],commandOutput,1000);
        printf("LEI\n");
        printf("%s\n",commandOutput);
    }

}