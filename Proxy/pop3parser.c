//
// Created by sswinnen on 17/10/18.
//

#include <stdlib.h>
#include <string.h>
#include "pop3parser.h"


static int clientFd, originServer, pipeliningSupported;
static int pipeFds[2];
static char ** envVars;

void attendClient(int clientSockFd, int originServerSock, char * envVariables [5]) {
    envVars = envVariables;
    startFilter();
    clientFd = clientSockFd;
    originServer = originServerSock;
    pipeliningSupported = pipeliningSupport(originServer);
    if(pipeliningSupported) {
        pipeliningMode();
    }
    else {
        noPipeliningMode();
    }
}

void pipeliningMode() {
    int clientReadIsFinished = FALSE;

    while(!clientReadIsFinished) {
        clientReadIsFinished = readFromClient();
    }
    writeAndReadFilter();
}

void noPipeliningMode() {
    ssize_t bytesRead;
    int clientReadIsFinished = FALSE;

    char buffer [BUFF_SIZE] = {0};

    while(!clientReadIsFinished) {
        if((bytesRead = read(clientFd,buffer, BUFF_SIZE)) == -1) {
            fprintf(stderr,"Read error");
            exit(EXIT_FAILURE);
        }
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
        if(cmdEnd == 0) {
            endFound = FALSE;
            write(originServer,buffer+cmdStart,chunkSize-cmdStart);
        }
        else {
            writeCommandEnd(buffer + cmdStart, cmdEnd - cmdStart);
            cmdStart = cmdEnd + 1;
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

    if((bytesRead = read(clientFd,buffer, BUFF_SIZE)) == -1) {
        fprintf(stderr,"Read error");
        exit(EXIT_FAILURE);
    }
    write(originServer,buffer,(size_t )bytesRead);
    if(bytesRead < BUFF_SIZE) {
        return TRUE;
    }
    return FALSE;
}


int readFromOrigin() {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};

    if ((bytesRead = read(clientFd, buffer, BUFF_SIZE)) == -1) {
        fprintf(stderr, "Read error");
        exit(EXIT_FAILURE);
    }
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
        fprintf(stderr, "Read error");
        exit(EXIT_FAILURE);
    }
    write(clientFd,buffer,(size_t)bytesRead);
    if(buffer[bytesRead-1] == EOF) {
        return TRUE;
    }
    return FALSE;
}



void startFilter() {
    int i;

    if(fork() == 0) {
        for(i = 0; i < 5; i++) {
            putenv(envVars[i]);
        }


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
            fprintf(stderr, "Error opening pipes to filter");
            exit(EXIT_FAILURE);
        }
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
        }
        memcpy(previousEnd,buffer + BUFF_SIZE - BUFFER_END, BUFFER_END);
    }
    return TRUE;

}

int findPipelining(char * str, ssize_t size) {
    int i = 0 , val = 0;
    size_t pipeliningLen = strlen("PIPELINING");

    do {
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