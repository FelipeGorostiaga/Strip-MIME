//
// Created by sswinnen on 11/10/18.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "constants.h"

enum state {NEXT_OPTION, LAST_OPTION};

typedef struct parsedConfiguration {
    char * options;
    char ** params;
} parsedConfiguration;

typedef parsedConfiguration * ParsedConfiguration;

/* Reads socket content and returns a structure with the parsed configuration. Returns success value or a closed socket value*/
int parseConfig(int socket);

/* Makes read call with error check */
int readSocket(int socket, char * buffer, size_t size);

/* Adds parsed options to the struct */
void updateConfigStruct(ParsedConfiguration pc, int index, char asciiValue, const char * param);

/* Frees all the allocated resources por parsed config structure */
void clearParsedConfigStruct(ParsedConfiguration pc, int maxIndex);

/* Edits configuration of pop3 filter */
void editConfiguration(ParsedConfiguration pcp);

