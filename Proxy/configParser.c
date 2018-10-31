//
// Created by sswinnen on 11/10/18.
//

#include "configParser.h"

extern int parseArgs(Configuration conf, int argc, char * argv []);

char metrics [BUFFER_SIZE];

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
        editConfiguration(config,opCode,(char *)buffer, socket, contentSize);
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

void editConfiguration(Configuration config, char opCode, char * buffer, int socket, ssize_t contentSize) {
    int ret = FALSE;
    uint8_t metrics [10] = {0};

    switch(opCode) {
        case 'e': ret = setErrorFile(config, buffer);
            break;
        case 'm': ret = setReplaceMessage(config, buffer);
            break;
        case 'M' : ret = setCensurableMediaTypes(config, buffer);
            break;
        case 't' : ret = setCommand(config, buffer);
            break;
        case 'z' : getMetrics(config, buffer, contentSize); write(socket,metrics,strlen((const char*)metrics));
            break;
        case 'x' : validatePassword(buffer,socket);
            break;
        default:
            break;
    }

    metrics[0] = (uint8_t)opCode;
    if(ret == TRUE) {
        metrics[1] = (uint8_t)2;
        metrics[2] = 'O';
        metrics[3] = 'K';
        write(socket,metrics,4);
    } else {
        metrics[1] = (uint8_t)3;
        metrics[2] = 'E';
        metrics[3] = 'R';
        metrics[4] = 'R';
        write(socket,metrics,5);
    }
}

int parseArguments(Configuration config, int argc, char * argv []) {
    return parseArgs(config,argc,argv);
}

void getMetrics(Configuration  config, const char * buffer, ssize_t contentSize) {
    int i, auxIndex, metricsIndex;
    char aux [BUFFER_SIZE] = {0};
    char * configData;

    auxIndex = 0; metricsIndex = 0;
    for(i = 0; i < contentSize; i++) {
        if(buffer[i] == ' ') {
            aux[auxIndex] = '\0';
            auxIndex = 0;
            if(strcmp(aux,"BYTES") == 0) {
                strcpy(metrics + metricsIndex, "BYTES ");
                metricsIndex += strlen("BYTES ");
                configData = getBytesTransferred(config);
            }
            else if(strcmp(aux,"CON_CON") == 0) {
                strcpy(metrics + metricsIndex, "CON_CON ");
                metricsIndex += strlen("CON_CON ");
                configData = getConcurrentConnections(config);
            }
            else if(strcmp(aux,"TOT_CON") == 0) {
                strcpy(metrics + metricsIndex, "TOT_CON ");
                metricsIndex += strlen("TOT_CON ");
                configData = getTotalAccesses(config);
            }
            else {
                return;
            }
            strcpy(metrics + metricsIndex, configData);
            metricsIndex += strlen(configData+1);
        }
        else {
            aux[auxIndex] = buffer[i];
        }
    }
    metrics[metricsIndex-1] = '\0';
}

void validatePassword(char * buffer, int socket) {
    uint8_t retBuff [BUFFER_SIZE];
    uint8_t len;
    retBuff[0] = 'x';

    if(strcmp(buffer,"adminpass") == 0) {
        len = 2;
        memcpy(retBuff + 1,&len,1);
        memcpy(retBuff + 2,"OK",len);
    }
    else {
        len = 5;
        memcpy(retBuff + 1,&len,1);
        memcpy(retBuff + 2,"ERROR",len);
    }
    write(socket,buffer,2+len);
}