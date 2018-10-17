//
// Created by sswinnen on 13/10/18.
//

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include "configuration.h"
#define END 0
#define KEEP_RUNNING 1

enum runningMode {VERSION_MODE, HELP_MODE, START_SERVER_MODE};

/* Analyzes syntax of args, no semantic analysis is provided*/
int parseArgs(Configuration conf, int argc, char * argv []);

/* Commands that start the server */
int parseServerStartCommands(Configuration conf,int argc,char *argv []);

/* Checks if the server must stop after printing info or not */
int getRunningMode(int argc, char * argv []);