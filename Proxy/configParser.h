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
#define END 0
#define KEEP_RUNNING 1

/* Reads socket content and returns a structure with the parsed configuration. Returns success value or a closed socket value*/
int parseConfig(int socket, Configuration config);

/* Makes read call with error check */
int readSocket(int socket, void * buffer, size_t size);

/* Edits configuration of pop3 filter */
void editConfiguration(Configuration config, char opCode, char * buffer, int socket, ssize_t contentSize);

/* Parses initial arguments*/
int parseArguments(Configuration config,int argc, char * argv []);

/* Parses content for metrics and returns buffer with values */
void getMetrics(Configuration  config, const char * buffer, ssize_t contentSize);

/*Receives password and writes response to client */
void validatePassword(char * buffer, int socket);
