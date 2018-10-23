//
// Created by sswinnen on 11/10/18.
//

#include "configParser.h"
extern int parseArgs(Configuration conf, int argc, char * argv []);

int parseConfig(int socket, Configuration config) {
    uint8_t * buffer = malloc(MAX_CONTENT_LEN* sizeof(uint8_t));
    char opCode;
    ssize_t bytesRead = 0, contentSize;
    uint8_t aux;

    do {
        contentSize = 0;
        bytesRead = readSocket(socket,&opCode,1);
        if(bytesRead == 0) { break; }
        do {
            bytesRead = readSocket(socket,&aux,1);
            if(bytesRead == 0) { break; }
            contentSize += aux;
        } while(aux != MAX_CONTENT_LEN);
        buffer = realloc(buffer,(contentSize+1)* sizeof(uint8_t));
        bytesRead = readSocket(socket,buffer,(size_t)contentSize);
        if(bytesRead == 0) { break; }
        buffer[contentSize] = 0;
        editConfiguration(config,opCode,(char *)buffer, socket);
    }
    while(TRUE);
    return SUCCESS;
}

int readSocket(int socket, void * buffer, size_t size) {
    ssize_t bytesRead;

    if((bytesRead = read(socket,buffer,size)) < 0) {
        fprintf(stderr,"Error reading socket");
        exit(EXIT_FAILURE);
    }
    return (int)bytesRead;
}

void editConfiguration(Configuration config, char opCode, char * buffer, int socket) {
    int ret = FALSE;
    uint8_t retBuffer [10] = {0};

    switch(opCode) {
        case 'e': ret = setErrorFile(config, buffer);
            break;
        case 'm': ret = setReplaceMessage(config, buffer);
            break;
        case 'M' : ret = setCensurableMediaTypes(config, buffer);
            break;
        case 't' : ret = setCommand(config, buffer);
            break;
        case 'z' : //getMetrics(config, buffer);
            break;
        default:
            break;
    }

    retBuffer[0] = (uint8_t)opCode;
    if(ret == TRUE) {
        retBuffer[1] = (uint8_t)2;
        retBuffer[2] = 'O';
        retBuffer[3] = 'K';
        write(socket,retBuffer,4);
    } else {
        retBuffer[1] = (uint8_t)3;
        retBuffer[2] = 'E';
        retBuffer[3] = 'R';
        retBuffer[4] = 'R';
        write(socket,retBuffer,5);
    }
}

int parseArguments(Configuration config,int argc, char * argv []) {
    return parseArgs(config,argc,argv);
}