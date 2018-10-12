//
// Created by sswinnen on 11/10/18.
//

#include "proxyParser.h"

int parseConfig(int socket) {
    char buffer [BUFFER_SIZE];
    int globalState = NEXT_OPTION, i = 0;
    char asciiValue;
    ssize_t bytesRead = 0, totalBytesRead = 0;
    size_t paramSize = 0, currentByte = 0;
    parsedConfiguration pc;

    if((pc.options = malloc(BLOCK_SIZE * sizeof(char))) == NULL) { exit(EXIT_FAILURE); }
    if((pc.params = malloc(BLOCK_SIZE * sizeof(char *))) == NULL) { exit(EXIT_FAILURE); }
    for(i = 0; globalState == NEXT_OPTION; i++) {
        bytesRead = readSocket(socket,buffer,1);
        if(bytesRead == 0) {
            return CLOSED_SOCKET;
        }
        totalBytesRead += bytesRead;
        asciiValue = buffer[0];
        if(asciiValue == 0) {
            globalState = LAST_OPTION;
        }
        bytesRead = readSocket(socket,buffer,1);
        totalBytesRead += bytesRead;
        currentByte = (uint8_t)buffer[0];
        paramSize = currentByte;
        while(currentByte != MAX_BYTE_VALUE) {
            bytesRead = readSocket(socket,buffer,1);
            totalBytesRead += bytesRead;
            currentByte = (uint8_t)buffer[0];
            paramSize += currentByte;
        }
        bytesRead = readSocket(socket,buffer,paramSize);
        totalBytesRead += bytesRead;
        updateConfigStruct(&pc, i, asciiValue, buffer);
    }
    editConfiguration(&pc);
    clearParsedConfigStruct(&pc, i);
    return SUCCESS;
}

int readSocket(int socket, char * buffer, size_t size) {
    ssize_t bytesRead;

    if((bytesRead = read(socket,buffer,size)) < 0) {
        exit(EXIT_FAILURE);
    }
    return (int)bytesRead;
}

void updateConfigStruct(ParsedConfiguration pc, int index, char asciiValue, const char * param) {
    ssize_t paramLen = strlen(param);

    if(index % BLOCK_SIZE == 0) {
        if((pc->options = realloc(pc->options, sizeof(char) * (index + BLOCK_SIZE))) == NULL) { exit(EXIT_FAILURE); }
        if((pc->params = realloc(pc->options, sizeof(char *) * (index + BLOCK_SIZE))) == NULL) { exit(EXIT_FAILURE); }
    }
    pc->options[index] = asciiValue;
    pc->params[index] = malloc(sizeof(char) * paramLen);
    strcpy(pc->params[index],param);
}

void clearParsedConfigStruct(ParsedConfiguration pc, int maxIndex) {
    int i;

    free(pc->options);
    for(i = 0; i <= maxIndex; i ++) {
        free(pc->params[i]);
    }
    free(pc->params);
}

void editConfiguration(ParsedConfiguration pcp) {
    //TODO
}