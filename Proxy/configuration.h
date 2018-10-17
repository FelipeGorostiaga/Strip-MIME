//
// Created by sswinnen on 13/10/18.
//
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <asm/errno.h>
#include <errno.h>
#include <regex.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netdb.h>

#define TRUE 1
#define FALSE 0

typedef struct configuration {
    /* -e */
    char * errorFile;
    /* -h */
    char * help;
    /* -l */
    in_addr_t pop3dir;
    /* -L */
    in_addr_t managDir;
    /* -m */
    char * replaceMessage;
    /* -M */
    char * censurableMediaTypes;
    /* -o */
    uint16_t managementPort;
    /* -p */
    uint16_t localPort;
    /* -P */
    uint16_t originPort;
    /* -t */
    char * command;
    /* -v */
    char * version;

    in_addr_t originServer;
} configuration;

typedef configuration * Configuration;

/* Creates new configuration structure */
Configuration newConfiguration();

/* Frees configuration resources */
void deleteConfiguration(Configuration config);

/* Checks if the string given is a valid port number */
int portIsValid(const char * port);

uint16_t strToUint16(const char * str);

/* Getters */
char * getErrorFile(Configuration conf);
char * getHelp(Configuration conf);
in_addr_t getPop3dir(Configuration conf);
in_addr_t getManagDir(Configuration conf);
char * getReplaceMessage(Configuration conf);
char * getCensurableMediaTypes(Configuration conf);
uint16_t getManagementPort(Configuration conf);
uint16_t getLocalPort(Configuration conf);
uint16_t getOriginPort(Configuration conf);
char * getCommand(Configuration conf);
char * getVersion(Configuration conf);
in_addr_t getOriginServer(Configuration conf);

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