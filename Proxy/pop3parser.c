//
// Created by sswinnen on 17/10/18.
//

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include "pop3parser.h"


static int clientFd, originServer, pipeliningSupported;
static int pipeFds[2];
static char ** envVars;
struct attendReturningFields rf;

struct attendReturningFields attendClient(int clientSockFd, int originServerSock, char * envVariables [5]) {
    rf.closeConnectionFlag = FALSE;
    rf.bytesTransferred = 0;
    envVars = envVariables;
    clientFd = clientSockFd;
    originServer = originServerSock;
    pipeliningSupported = pipeliningSupport(originServer);
    if(pipeliningSupported) {
        pipeliningMode();
    }
    else {
        noPipeliningMode();
    }
    printf("Returning attend client\n");
    return rf;
}

void pipeliningMode() {
    int clientReadIsFinished = FALSE;

    while(!clientReadIsFinished) {
        clientReadIsFinished = readFromClient();
        printf("Just read form client client read %s finished\n",clientReadIsFinished==TRUE?"is":"isn't");
    }

    writeAndReadFilter();
}

void noPipeliningMode() {
    ssize_t bytesRead;
    int clientReadIsFinished = FALSE;

    char buffer [BUFF_SIZE] = {0};

    while(!clientReadIsFinished) {
        if((bytesRead = read(clientFd,buffer, BUFF_SIZE)) == -1) {
            fprintf(stderr,"Read error in no pipelining mode");
            exit(EXIT_FAILURE);
        }
        rf.bytesTransferred += bytesRead;
        if(bytesRead < BUFF_SIZE) {
            clientReadIsFinished = TRUE;
        }
        parseChunk(buffer, bytesRead);
    }
}

void parseChunk(char * buffer, ssize_t chunkSize) {
    size_t cmdStart = 0, cmdEnd = 0;
    int endFound = TRUE;

    while(endFound) {
        cmdEnd = commandEnd(buffer+cmdStart,chunkSize);
        if(cmdEnd == NOT_FOUND) {
            endFound = FALSE;
            write(originServer,buffer+cmdStart,chunkSize-cmdStart);
        }
        else {
            writeCommandEnd(buffer + cmdStart, cmdEnd - cmdStart);
            cmdStart = cmdEnd + 1;
            logAccess(buffer,cmdStart);
            if(commandsAreEqual(buffer+cmdStart,"quit")) {
                rf.closeConnectionFlag = TRUE;
            }
        }
    }

}

void writeCommandEnd(char * buffer, size_t len) {
    int originReadIsFinished = FALSE;

    write(originServer,buffer,len);
    while(!originReadIsFinished) {
        originReadIsFinished = readFromOrigin();
    }
    writeAndReadFilter();
}

void writeAndReadFilter() {
    int originReadIsFinished = FALSE, filterReadIsFinished = FALSE;

    while(!originReadIsFinished || !filterReadIsFinished) {
        if(!originReadIsFinished) {
            originReadIsFinished = readFromOrigin();
        }
        if(!filterReadIsFinished) {
            filterReadIsFinished = readFromFilter();
        }
    }
}

int readFromClient() {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};
    size_t i;

    if((bytesRead = read(clientFd,buffer, BUFF_SIZE-1)) == -1) {
        fprintf(stderr,"Error reading from client\n");
        exit(EXIT_FAILURE);
    }
    printf("Successful read from client\n");
    rf.bytesTransferred += bytesRead;
    for(i = 1; (i + 4) < bytesRead; i++) {
        if(buffer[i-1] == '\r' && buffer[i] == '\n') {
            logAccess(buffer,i+1);
        }
    }
    buffer[bytesRead] = 0;
    printf("Buffer size is %d an bytesRead is %d\n", BUFF_SIZE-1, (uint)bytesRead);
    printf("Read: %s", buffer);
    write(originServer,buffer,(size_t )bytesRead-2);
    write(originServer,"\r\n",2);
    if(bytesRead < BUFF_SIZE) {
        return TRUE;
    }
    return FALSE;
}


int readFromOrigin() {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};
    if ((bytesRead = read(originServer, buffer, BUFF_SIZE)) == -1) {
        fprintf(stderr, "Error reading from origin\n");
        exit(EXIT_FAILURE);
    }
    rf.bytesTransferred += bytesRead;
    write(pipeFds[1],buffer,(size_t)bytesRead);
    if(bytesRead < BUFF_SIZE) {
        return TRUE;
    }
    return FALSE;
}

int readFromFilter() {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};

    if ((bytesRead = read(pipeFds[0], buffer, BUFF_SIZE)) == -1) {
        fprintf(stderr, "Error reading from filter\n");
        exit(EXIT_FAILURE);
    }
    buffer[bytesRead] = 0;
    printf("Read the following from filter: %s\n", buffer);
    write(clientFd,buffer,(size_t)bytesRead);
    if(buffer[bytesRead-2] == '\r') {
        return TRUE;
    }
    return FALSE;
}



void startFilter() {
    int i;
    char *argv[] = {"echo", 0};

    if(fork() == 0) {
        for(i = 0; i < 5; i++) {
            putenv(envVars[i]);
        }
        dup2(pipeFds[1],STDIN_FILENO);
        dup2(pipeFds[0],STDOUT_FILENO);
        execvp(argv[0],argv);
    }
    else {
        if(pipe(pipeFds) == -1) {
            fprintf(stderr,"Error opening pipes to filter");
            exit(EXIT_FAILURE);
        }
    }
}

int pipeliningSupport(int originServer) {
    char buffer [BUFF_SIZE];
    char extendedBuff[BUFF_SIZE+BUFFER_END];
    char previousEnd[BUFFER_END];
    int found = FALSE, firstRead = TRUE;
    ssize_t bytesRead;

    write(originServer,"CAPA\r\n",6);
    while(!found) {
        if ((bytesRead = read(originServer, buffer, BUFF_SIZE)) == -1) {
            fprintf(stderr, "Error reading capabilities from origin server");
            exit(EXIT_FAILURE);
        }
        buffer[bytesRead] = '\0';
        if (bytesRead < BUFF_SIZE) {
            if(firstRead) {
                 return findPipelining(buffer, bytesRead);
            }
            else {
                memcpy(extendedBuff,previousEnd,BUFFER_END);
                memcpy(extendedBuff+BUFFER_END,buffer,(size_t)bytesRead);
                return findPipelining(extendedBuff, BUFFER_END +bytesRead);
            }
        }
        if(firstRead) {
            found = findPipelining(buffer, BUFF_SIZE);
            firstRead = FALSE;
        }
        else {
            memcpy(extendedBuff,previousEnd,BUFFER_END);
            memcpy(extendedBuff+BUFFER_END,buffer,BUFF_SIZE);
            found = findPipelining(extendedBuff, BUFFER_END + BUFF_SIZE);
            if(found == NOT_SUPPORTED) {
                return FALSE;
            }
        }
        memcpy(previousEnd,buffer + BUFF_SIZE - BUFFER_END, BUFFER_END);
    }
    return TRUE;

}

int findPipelining(char * str, ssize_t size) {
    int i = 0 , val = 0;
    size_t pipeliningLen = strlen("PIPELINING");

    do {
        if(str[i] == '\r' && str[i+1] == '\n') {
            return NOT_SUPPORTED;
        }
        val = strncmp(str + i, "PIPELINING",pipeliningLen);
        if(val == 0) {
            return TRUE;
        }
        while(str[i++] != '\n' && i < size);
    } while(i < size);
    return FALSE;
}

size_t commandEnd(const char * buffer, ssize_t size) {
    size_t i;
    int rFound;

    for(i = 0; i < size; i ++) {
        if(buffer[i] == '\r') {
            rFound = TRUE;
        }
        else {
            rFound = FALSE;
        }
        if(buffer[i] == '\n' && rFound) {
            return i;
        }
    }
    return 0;
}

void logAccess(char * buffer, size_t cmdStart) {
    char cmd [5] = {0};

    memcpy(cmd,buffer+cmdStart,4* sizeof(char));
    fprintf(stdout,"%lu ", (unsigned long)time(NULL));
    fprintf(stdout,"id: %d ", clientFd);
    fprintf(stdout,"%s\n",cmd);

}

int commandsAreEqual(const char * command1, const char * command2) {
    char aux1[5] = {0};
    char aux2[5] = {0};
    int i;

    memcpy(aux1,command1,4* sizeof(char));
    memcpy(aux2,command2,4* sizeof(char));

    for(i = 0; i < 4; i++) {
        aux1[i] = (char)tolower(aux1[i]);
        aux2[i] = (char)tolower(aux2[i]);
    }

    return strncmp(aux1,aux2,4) == 0 ? TRUE : FALSE;
}
