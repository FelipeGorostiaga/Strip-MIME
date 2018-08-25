#define BLOCK 100
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

typedef void (*slaveFunc)(int * outputFds, char * command);

char * concatArgs(int argc, char * argv[]);
void createSlave(slaveFunc sFunc, char * command);
void executeCommand(int * outputFds, char * command);
void inputParity(int * outputFds, char * command);
int parity(const char * str);

int main(int argc, char * argv []) {

    char * command = NULL;
    char commandOutput[1000] = {0};
    size_t cmdLength = 0;
    ssize_t bytesRead;
    size_t stdInLen = 0;

    if(argc <= 1)
        return 0;

    command = concatArgs(argc,argv);

    createSlave(inputParity,NULL);
    createSlave(executeCommand,command);

    scanf("%s", commandOutput); /*after child writes in parent's stdin*/

    fprintf(stderr,"Output parity: %d\n",parity(commandOutput));
    printf("%s\n", commandOutput);
}

/*Concatenates command line arguments*/
char * concatArgs(int argc, char * argv[]) {

    int i, j;
    size_t cmdLength = 0;
    char * command = NULL;

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

    return command;
}

void createSlave(slaveFunc sFunc, char * command) {

    int outputFds[2] = {-1,-1};
    int status;

    if(pipe(outputFds) < 0) { exit(1); }

    if(fork() != 0) {
        close(outputFds[1]); /*closes write end of the pipe*/
        dup2(outputFds[0], 0);
        while(wait(&status) > 0);
    }
    else {
        sFunc(outputFds,command);
        exit(0);
    }
}

void inputParity(int * outputFds, char * command) {
    char stdInput[1000];

    close(outputFds[0]);
    dup2(outputFds[1], 1);

    scanf("%s",stdInput);
    printf("%s", stdInput);
    fprintf(stderr,"Input parity: %d\n",parity(stdInput));
}

int parity(const char * str) {
    int i, j, sum = 0;
    for(i = 0; str[i] != 0; i++) {
        for(j = 0; j < 8; j++) {
            sum += str[i] & (1 << j);
        }
    }
    return sum;
}

void executeCommand(int * outputFds, char * command) {
    close(outputFds[0]);
    dup2(outputFds[1], 1);
    execl("/bin/sh", "/bin/sh", "-c", command, NULL);
}