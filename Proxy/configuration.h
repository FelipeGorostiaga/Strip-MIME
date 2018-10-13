//
// Created by sswinnen on 13/10/18.
//
#include <stdlib.h>

typedef struct configuration {
    /* -e */
    char * errorFile;
    /* -h */
    char * help;
    /* -l */
    char * pop3dir;
    /* -L */
    char * managDir;
    /* -m */
    char * replaceMessage;
    /* -M */
    char * censurableMediaTypes;
    /* -o */
    char * managementPort;
    /* -p */
    char * localPort;
    /* -P */
    char * originPort;
    /* -t */
    char * command;
    /* -v */
    char * version;

    char * originServer;
} configuration;

typedef configuration * Configuration;

/* Creates new configuration structure */
Configuration newConfiguration();

/* Frees configuration resources */
void deleteConfiguration(Configuration config);


/* Getters */
char * getErrorFile(Configuration conf);
char * getHelp(Configuration conf);
char * getPop3dir(Configuration conf);
char * getManagDir(Configuration conf);
char * getReplaceMessage(Configuration conf);
char * getCensurableMediaTypes(Configuration conf);
char * getManagementPort(Configuration conf);
char * getLocalPort(Configuration conf);
char * getOriginPort(Configuration conf);
char * getCommand(Configuration conf);
char * getVersion(Configuration conf);
char * getOriginServer(Configuration conf);

/*Setters*/
void setErrorFile(Configuration conf, char * errorFile);
void setPop3dir(Configuration conf, char * pop3dir);
void setManagDir(Configuration conf, char * managDir);
void setReplaceMessage(Configuration conf, char * replaceMessage);
void setCensurableMediaTypes(Configuration conf, char * censurableMediaTypes);
void setManagementPort(Configuration conf, char * managementPort);
void setLocalPort(Configuration conf, char * localPort);
void setOriginPort(Configuration conf, char * originPort);
void setCommand(Configuration conf, char * command);
void setOriginServer(Configuration conf, char * originServer);