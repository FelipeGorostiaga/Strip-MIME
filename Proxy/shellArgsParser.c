//
// Created by sswinnen on 13/10/18.
//

#include "shellArgsParser.h"

void parseArgs(Configuration conf, int argc,  char * argv []) {
    int runningMode = getRunningMode(argc,argv);

    if(runningMode == VERSION_MODE) {
        printf("Version: %s\n", getVersion(conf));
    } else if(runningMode == HELP_MODE) {
        printf("%s\n", getHelp(conf));
    } else {
        parseServerStartCommands(conf, argc, argv);
    }
}

void parseServerStartCommands(Configuration conf, int argc, char *argv []) {
    int i;
    char option;

    if(argc % 2 != 0) {
        perror("Invalid argument syntax\n"); exit(1);
    }

    for(i = 1; i < argc-2; i += 2) {
        if(i == argc-1) {
            setOriginServer(conf,argv[i]);
        }
        option = getParamLetter(argv[i]);
        switch(option) {
            case 'e': setErrorFile(conf, argv[i+1]);
                break;
            case 'l': setPop3dir(conf, argv[i+1]);
                break;
            case 'L': setManagDir(conf, argv[i+1]);
                break;
            case 'm': setReplaceMessage(conf, argv[i+1]);
                break;
            case 'M' : setCensurableMediaTypes(conf, argv[i+1]);
                break;
            case 'o' : setManagementPort(conf, argv[i+1]);
                break;
            case 'p' : setLocalPort(conf, argv[i+1]);
                break;
            case 'P' : setOriginPort(conf, argv[i+1]);
                break;
            case 't' : setCommand(conf, argv[i+1]);
                break;
            default  : perror("Invalid argument syntax\n"); exit(1);
        }
    }
    setOriginServer(conf,argv[argc-1]);
}

char getParamLetter(char * param) {
    if(strlen(param) == 2 && param[0] == '-') {
        return param[1];
    }
    perror("Invalid argument syntax\n"); exit(1);
}

int getRunningMode(int argc, char * argv []) {
    if(argc == 2) {
        if (strcmp(argv[1], "-v") == 0) {
            return VERSION_MODE;
        } else if (strcmp(argv[1], "-h") == 0) {
            return HELP_MODE;
        }
    }
    return START_SERVER_MODE;
}