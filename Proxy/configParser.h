//
// Created by sswinnen on 11/10/18.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "constants.h"
#include "configuration.h"

/* Reads socket content and returns a structure with the parsed configuration. Returns success value or a closed socket value*/
int parseConfig(int socket, Configuration config);

/* Makes read call with error check */
int readSocket(int socket, void * buffer, size_t size);

/* Edits configuration of pop3 filter */
void editConfiguration(Configuration config, char opCode, char * buffer, int socket);

