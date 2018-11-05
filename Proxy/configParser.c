//
// Created by sswinnen on 11/10/18.
//

#include "configParser.h"

extern int parseArgs(Configuration conf, int argc, char * argv []);

char * metrics;

int parseConfig(int socket, Configuration config) {
    uint8_t * buffer = malloc(MAX_CONTENT_LEN* sizeof(uint8_t));
    char opCode;
    ssize_t bytesRead = 0, contentSize;
    uint8_t aux;
    contentSize = 0;
    metrics = calloc(BUFFER_SIZE, sizeof(char));
    readSocket(socket,&opCode,1);
    printf("opcode: %c\n",opCode);
    do {
        bytesRead = readSocket(socket,&aux,1);
        if(bytesRead == 0) {
            close(socket);
            return CLOSED_SOCKET;
        }
        printf("length: %d\n",aux);
        contentSize += aux;
    } while(aux == MAX_CONTENT_LEN);
    buffer = realloc(buffer,(contentSize+1)* sizeof(uint8_t));
    bytesRead = readSocket(socket,buffer,(size_t)contentSize);
    buffer[bytesRead] = 0;
    printf("content: %s\n",buffer);
    buffer[contentSize] = 0;
    editConfiguration(config,opCode,(char *)buffer, socket, contentSize);
    free(buffer);
    free(metrics);
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
    uint8_t len;
    uint8_t response [10] = {0};

    if(opCode == 'v') {
        char * vers = getVersion(config);
        len = (uint8_t )strlen(vers);
        response[0] = 'v';
        response[1] = len;
        memcpy(response+2,vers,len);
        write(socket,response,len+2);
        return;
    }

    switch(opCode) {
        case 'e': ret = setErrorFile(config, buffer);
            break;
        case 'm': ret = setReplaceMessage(config, buffer);
            break;
        case 'M' : ret = setCensurableMediaTypes(config, buffer);
            break;
        case 't' : ret = setCommand(config, buffer);
            break;
        case 'z' : getMetrics(config, buffer, contentSize);
            printf("Writing: %s\n",metrics);
            write(socket,metrics,strlen((const char*)metrics));
            return;
        case 'x' : ret = validatePassword(buffer,socket);
            break;
        default:
            break;
    }

    response[0] = (uint8_t)opCode;
    if(ret == TRUE) {
        len = 2;
        response[1] = len;
        memcpy(response + 2,"OK",len);
    }
    else {
        len = 5;
        response[1] = len;
        memcpy(response + 2,"ERROR",len);
    }
    write(socket,response,2+len);

}

int parseArguments(Configuration config, int argc, char * argv []) {
    return parseArgs(config,argc,argv);
}

void clearAux(char * aux) {
    int i;
    for(i = 0; i < BUFFER_SIZE; i++) {
        aux[i] = 0;
    }
}

void getMetrics(Configuration  config, const char * buffer, ssize_t contentSize) {
    int i, auxIndex, metricsIndex;
    char aux [BUFFER_SIZE] = {0};
    char * configData;

    metrics[0] = 'z';
    metrics[1] = 'z';
    auxIndex = 0; metricsIndex = 2;
    for(i = 0; i < contentSize+1 ; i++) {
        if(buffer[i] == ' ' || i == contentSize) {
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
                configData = "NULL";
            }
            strcpy(metrics + metricsIndex, configData);
            metricsIndex += strlen(configData);
            if(i != contentSize) {
                metrics[metricsIndex++] = ' ';
            }
            clearAux(aux);
        }
        else {
            aux[auxIndex++] = buffer[i];
        }

    }
    metrics[1] = (char)((uint8_t )strlen(metrics) -2);
}

int validatePassword(char * buffer, int socket) {

    if(strcmp(buffer,"adminpass") == 0) {
        return TRUE;
    }
    return FALSE;
}