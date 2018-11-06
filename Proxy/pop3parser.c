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


static int clientFd, originServer, pipeliningSupported, requestsNum, responsesNum, filteredResponses;

struct attendReturningFields rf;

struct attendReturningFields attendClient(int clientSockFd, int originServerSock) {
    rf.closeConnectionFlag = FALSE;
    rf.bytesTransferred = 0;
    clientFd = clientSockFd;
    originServer = originServerSock;
    pipeliningSupported = pipeliningSupport(originServer);
    if(pipeliningSupported) {
        pipeliningMode(clientSockFd,originServerSock);
    }
    else {
        //noPipeliningMode();
    }
    return rf;
}

void pipeliningMode(int client, int originServer) {

    int ret = readFromClient(client,originServer);

    if(ret == QUIT || ret == QUITCOMMAND) {
        rf.closeConnectionFlag = TRUE;
        if(ret == QUIT) {
            return;
        }
    }
}

//void noPipeliningMode() {
//    ssize_t bytesRead;
//    char buffer [BUFF_SIZE] = {0};
//
//    while(TRUE) {
//        if((bytesRead = read(clientFd,buffer, BUFF_SIZE)) == -1) {
//            if(errno == EWOULDBLOCK) {
//                return;
//            }
//            fprintf(stderr,"Read error in no pipelining mode");
//            exit(EXIT_FAILURE);
//        }
//        if(bytesRead == 0) {
//            rf.closeConnectionFlag = TRUE;
//            return;
//        }
//        rf.bytesTransferred += bytesRead;
//        parseChunk(buffer, bytesRead);
//    }
//}

//void parseChunk(char * buffer, ssize_t chunkSize) {
//    int cmdStart = 0, cmdEnd = 0;
//    int endWithNewline = FALSE;
//    int originClosed;
//    int i = 0;
//
//    if(requestsNum == 0) {
//        logAccess(buffer,(size_t )cmdStart);
//    }
//    for(i = 0; i < chunkSize; i++) {
//        if(buffer[i] == '\n') {
//            cmdEnd = i;
//            requestsNum += 1;
//            cleanAndSend(buffer,cmdStart,cmdEnd);
//            originClosed = writeAndReadFilter();
//            if(originClosed) {
//                rf.closeConnectionFlag = TRUE;
//                return;
//            }
//            cmdStart = cmdEnd + 1;
//            if(i == chunkSize-1) {
//                endWithNewline = TRUE;
//            } else {
//                logAccess(buffer,(size_t )cmdStart);
//            }
//        }
//    }
//    if(!endWithNewline) {
//        write(originServer,buffer+cmdStart, (size_t)chunkSize-cmdStart);
//    }
//
//}


int readFromClient(int client,int originServer) {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};
    size_t i;

    if((bytesRead = read(client,buffer, BUFF_SIZE)) == -1) {
        if(errno == EWOULDBLOCK) {
            return TRUE;
        }
        fprintf(stderr,"Error reading from client\n");
        exit(EXIT_FAILURE);
    }
    if(bytesRead == 0) {
        return QUIT;
    }
    printf("Writing to origin server: %s\n",buffer);
    write(originServer,buffer,(size_t)bytesRead);
    logAccess(buffer,0);
    rf.bytesTransferred += bytesRead;
    for(i = 0; i < bytesRead; i++) {
        if(buffer[i] == '\n') {
            requestsNum += 1;
            logAccess(buffer,i+1);
        }
    }
    return FALSE;
}


int readFromOrigin(int originServer,int filterInput) {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};
    if ((bytesRead = read(originServer, buffer, BUFF_SIZE)) == -1) {
        if(errno == EWOULDBLOCK) {
            return TRUE;
        }
        fprintf(stderr, "Error reading from origin\n");
        exit(EXIT_FAILURE);
    }
    if(bytesRead == 0) {
        return QUIT;
    }
    responsesNum += countResponses(buffer,bytesRead);
    rf.bytesTransferred += bytesRead;
    write(filterInput,buffer,(size_t)bytesRead);
    return TRUE;
}

int readFromFilter(int filterOutput, int clientFd) {
    ssize_t bytesRead;
    char buffer [BUFF_SIZE] = {0};

    if ((bytesRead = read(filterOutput, buffer, BUFF_SIZE)) == -1) {
        fprintf(stderr, "Error reading from filter\n");
        if(errno == EWOULDBLOCK) {
            return TRUE;
        }
        exit(EXIT_FAILURE);
    } else if(bytesRead == 0) {
        return QUIT;
    }
    printf("LEI DEL FILTRO: %s\n", buffer);
    filteredResponses += countResponses(buffer,bytesRead);
    write(clientFd,buffer,(size_t)bytesRead);
    return TRUE;
}



int * startFilter(char ** envVariables) {
    int i;
    char *argv[] = {"./stripmime", 0};
    int * pipes = malloc(sizeof(int)*2);

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
        execvp(argv[0],argv);
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

int countResponses(char * buf, ssize_t size) {
    int i = 0, count = 0, mailEndingParse = FALSE;

    for(i = 0; i < size; i++) {
        if(!mailEndingParse && buf[i] == '\r' && buf[i+1] == '\n'
           && ((i + 5 >= size && size != BUFF_SIZE) || (buf[i+2] == '.' && buf[i+3] == '\r' && buf[i+4] == '\n'))) {
            mailEndingParse = TRUE;
            count++;
        }
        if(strncasecmp(buf+i,"+OK",3) == 0) {
            logResponse("OK\0");
        } else if(strncasecmp(buf+i,"-ERR",4) == 0) {
            logResponse("ERR\0");
        }
    }
    return count;
}

int cleanAndSend(const char * buffer, int cmdStart, int cmdEnd) {
    char aux [BUFF_SIZE];
    int i = cmdStart;
    int j = 0, k = 0;
    while(i <= cmdEnd) {
        aux[j] = buffer[i];
        i++;
        j++;
    }
    aux[j] = 0;
    for(k = 0; aux[j - k - 3] == ' ' &&  k < (cmdEnd - cmdStart); k++);
    if( k != 0 ) {
        aux[j - k - 2] = '\r';
        aux[j - k - 1] = '\n';
        aux[j - k] = '0';
    }
    if(write(originServer,aux,(size_t) (j-k)) == 0) {
        return QUIT;
    }
    if(strncasecmp((buffer+cmdStart),"QUIT",4) == 0) {
        return QUITCOMMAND;
    }
    return TRUE;
}

void logResponse(char * buffer) {
    char buff[100];
    time_t now = time (0);
    strftime (buff, 100, "%Y-%m-%d %H:%M:%S", localtime (&now));
    fprintf(stdout,"%s ", buff);
    fprintf(stdout,"server response: %s\n", buffer);
}