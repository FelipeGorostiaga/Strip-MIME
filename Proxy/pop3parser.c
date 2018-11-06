//
// Created by sswinnen on 17/10/18.
//

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "pop3parser.h"


static int clientFd, originServer, pipeliningSupported;

struct attendReturningFields rf;

struct attendReturningFields attendClient(int clientSockFd, int originServerSock) {
    rf.closeConnectionFlag = FALSE;
    rf.bytesTransferred = 0;
    clientFd = clientSockFd;
    originServer = originServerSock;
    pipeliningSupported = pipeliningSupport(originServer);
    if(pipeliningSupported == QUIT) {
        rf.closeConnectionFlag = QUIT;
        return rf;
    }
    if(pipeliningSupported) {
        pipeliningMode(clientSockFd,originServerSock);
    }
    else {
        rf.closeConnectionFlag = QUIT;
    }
    return rf;
}

void pipeliningMode(int client, int originServer) {

    int ret = readFromClient(client,originServer);

    if(ret == QUIT) {
        rf.closeConnectionFlag = QUIT;
    }
    if(ret == QUITCOMMAND) {
        rf.closeConnectionFlag = QUITCOMMAND;
    }
}


int readFromClient(int client,int originServer) {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};
    size_t i;
    int ret = TRUE;

    while(TRUE) {
        if((bytesRead = read(client,buffer, BUFF_SIZE)) == -1) {
            if(errno == EWOULDBLOCK) {
                return ret;
            }
            return QUIT;
        }
        if(bytesRead == 0) {
            return QUIT;
        }
        write(originServer,buffer,(size_t)bytesRead);
        logAccess(buffer,0);
        rf.bytesTransferred += bytesRead;
        for(i = 0; i < bytesRead; i++) {
            if(buffer[i] == '\n' && i != bytesRead-1) {
                logAccess(buffer,i+1);
            }
        }
    }

}


int readFromOrigin(int originServer,int filterInput) {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};

    if ((bytesRead = read(originServer, buffer, BUFF_SIZE)) == -1) {
        if(errno == EWOULDBLOCK) {
            return TRUE;
        }
        return QUIT;
    }
    if(bytesRead == 0) {
        return QUITCOMMAND;
    }
    if(strncasecmp(buffer,"-ERR Disconnected: Shutting down\r\n",(size_t )bytesRead) == 0) {
        return QUIT;
    }
    write(filterInput,buffer,(size_t)bytesRead);
    return 0;
}

int readFromFilter(int filterOutput, int clientFd) {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};
    while(TRUE) {
        if ((bytesRead = read(filterOutput, buffer, BUFF_SIZE)) == -1) {
            if(errno == EWOULDBLOCK) {
                return TRUE;
            }
            return QUIT;
        } else if(bytesRead == 0) {
            return QUIT;
        }
        write(clientFd,buffer,(size_t)bytesRead);
        return TRUE;
    }
}



int * startFilter(char * command, char ** envVariables) {
    int i;
    char *argv[] = {command, 0};
    int * pipes = malloc(sizeof(int)*2);
    printf("Starting %s\n",command);
    int parentInputPipeFds[2];
    int parentOutputPipeFds[2];

    int ret1 = pipe(parentInputPipeFds);
    int ret2 = pipe(parentOutputPipeFds);

    pipes[0] = parentInputPipeFds[0];
    pipes[1] = parentOutputPipeFds[1];

    if(fork() == 0) {
        for(i = 0; i < 5; i++) {
            putenv(envVariables[i]);
        }
        dup2(parentOutputPipeFds[0],STDIN_FILENO);
        dup2(parentInputPipeFds[1],STDOUT_FILENO);
        if(execvp(argv[0],argv) < 0) {
            fprintf(stderr,"Invalid command, executing cat\n");
            execvp("cat",argv);
        }
    }
    else {
        if(ret1 == -1) {
            fprintf(stderr,"Error opening pipes to filter");
            exit(EXIT_FAILURE);
        }
        if(ret2 == -1) {
            fprintf(stderr,"Error opening pipes to filter");
            exit(EXIT_FAILURE);
        }
    }
    return pipes;
}

int pipeliningSupport(int originServer) {
    char buffer [BUFF_SIZE];
    char extendedBuff[BUFF_SIZE+BUFFER_END];
    char previousEnd[BUFFER_END];
    int found = FALSE, firstRead = TRUE;
    ssize_t bytesRead;
    ssize_t writen;

    if((writen = write(originServer,"CAPA\r\n",6)) == 0) {
        return QUIT;
    }
    if(writen == -1) {
        return QUIT;
    }
    while(!found) {
        if ((bytesRead = read(originServer, buffer, BUFF_SIZE)) == -1 || bytesRead == 0) {
            return QUIT;
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

void logAccess(char * buffer, size_t cmdStart) {
    char cmd [5] = {0};
    char buff[100];
    time_t now = time (0);
    strftime (buff, 100, "%Y-%m-%d %H:%M:%S", localtime (&now));
    fprintf(stdout,"%s ", buff);
    memcpy(cmd,buffer+cmdStart,4* sizeof(char));
    fprintf(stdout,"id: %d ", clientFd);
    fprintf(stdout,"%s\n",cmd);

}
