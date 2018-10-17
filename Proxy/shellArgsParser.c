//
// Created by sswinnen on 13/10/18.
//


#include "shellArgsParser.h"

int parseArgs(Configuration conf, int argc,  char * argv []) {
    int runningMode = getRunningMode(argc,argv);

    if(runningMode == VERSION_MODE) {
        printf("Version: %s\n", getVersion(conf));
        return END;
    } else if(runningMode == HELP_MODE) {
        printf("%s\n", getHelp(conf));
        return END;
    } else {
        return parseServerStartCommands(conf, argc, argv);
    }
}

int parseServerStartCommands(Configuration conf, int argc, char *argv []) {
    int option;

    while((option = getopt(argc, argv,"e:l:L:m:M:o:p:P:t:")) != -1) {
        switch(option) {
            case 'e': setErrorFile(conf, optarg);
                break;
            case 'l': setPop3dir(conf, optarg);
                break;
            case 'L': setManagDir(conf, optarg);
                break;
            case 'm': setReplaceMessage(conf, optarg);
                break;
            case 'M' : setCensurableMediaTypes(conf, optarg);
                break;
            case 'o' : setManagementPort(conf, optarg);
                break;
            case 'p' : setLocalPort(conf, optarg);
                break;
            case 'P' : setOriginPort(conf, optarg);
                break;
            case 't' : setCommand(conf, optarg);
                break;
            case '?' : {
                if (optopt == 'e' || optopt == 'l' || optopt == 'L' || optopt == 'm' || optopt == 'M'
                    || optopt == 'o' || optopt == 'p' || optopt == 'P' || optopt == 't') {
                    fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
                }
                else if (optopt == 'v' || optopt == 'h') {
                    fprintf (stderr, "La opción -%c no es válida en este contexto\n", optopt);
                }
                else if (isprint (optopt)) {
                    fprintf (stderr, "Opción desconocida: `-%c'.\n", optopt);
                }
                else {
                    fprintf (stderr, "Caracter de opción desconocido: `\\x%x'.\n", optopt);
                }
                return END;
            }
        }
    }
    setOriginServer(conf,argv[argc-1]);
    return KEEP_RUNNING;
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