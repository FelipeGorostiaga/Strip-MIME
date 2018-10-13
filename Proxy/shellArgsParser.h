//
// Created by sswinnen on 13/10/18.
//

#include <string.h>
#include <stdio.h>
#include "configuration.h"

enum runningMode {VERSION_MODE, HELP_MODE, START_SERVER_MODE};

/* Analyzes syntax of args, no semantic analysis is provided*/
void parseArgs(Configuration conf, int argc, char * argv []);

/* Commands that start the server */
void parseServerStartCommands(Configuration conf,int argc,char *argv []);

/* Checks if the server must stop after printing info or not */
int getRunningMode(int argc, char * argv []);

char getParamLetter(char * param);