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
    struct in_addr pop3dir;
    struct in6_addr pop3dir6;
    int pop3dirFamily;
    /* -L */
    struct in_addr managDir;
    struct in6_addr managDir6;
    int managDirFamily;
    /* -m */
    char * replaceMessage;
    /* -M */
    char  censurableMediaTypes [256];
    /* -o */
    uint16_t managementPort;
    /* -p */
    uint16_t localPort;
    /* -P */
    uint16_t originPort;
    /* -t */
    char command [256];
    /* -v */
    char * version;
    /* -z */
    long concurrentConnections;
    long bytesTransferred;
    long totalAccesses;
    char * currentUser;
    char * originServerString;
    in_addr_t originServer;
    int originServerIsActive;
} configuration;

typedef configuration * Configuration;

/* Creates new configuration structure */
Configuration newConfiguration();

/* Frees configuration resources */
void deleteConfiguration(Configuration config);

/* Checks if the string given is a valid port number */
int portIsValid(const char * port);

void newAccess(Configuration conf);

void newConcurrentConnection(Configuration conf);

void closedConcurrentConnection(Configuration conf);

void addBytesTransferred(Configuration conf, long bytes);


uint16_t strToUint16(const char * str);

int getFamily(char * dir);

/* Getters */
char * getErrorFile(Configuration conf);
char * getHelp(Configuration conf);
struct in_addr getPop3dir(Configuration conf);
struct in6_addr getPop3dir6(Configuration conf);
struct in_addr getManagDir(Configuration conf);
struct in6_addr getManagDir6(Configuration conf);
char * getReplaceMessage(Configuration conf);
char * getCensurableMediaTypes(Configuration conf);
uint16_t getManagementPort(Configuration conf);
uint16_t getLocalPort(Configuration conf);
uint16_t getOriginPort(Configuration conf);
char * getCommand(Configuration conf);
char * getVersion(Configuration conf);
in_addr_t getOriginServer(Configuration conf);
int getOriginServerIsActive(Configuration conf);
char * getOriginServerString(Configuration conf);
char * getCurrentUser(Configuration conf);
char * getTotalAccesses(Configuration conf) ;
char * getBytesTransferred(Configuration conf);
char * getConcurrentConnections(Configuration conf);
int getPopDirFamily(Configuration conf);
int getManagDirFamily(Configuration conf);

/*Setters*/
int setErrorFile(Configuration conf, char * errorFile);
void setPop3dir(Configuration conf, char * pop3dir);
void setManagDir(Configuration conf, char * managDir);
int setReplaceMessage(Configuration conf, char * replaceMessage);
int setCensurableMediaTypes(Configuration conf, char * censurableMediaTypes);
void setManagementPort(Configuration conf, char * managementPort);
void setLocalPort(Configuration conf, char * localPort);
void setOriginPort(Configuration conf, char * originPort);
int setCommand(Configuration conf, char * command);
void setOriginServer(Configuration conf, char * originServer);
void setOriginServerIsActive(Configuration conf,int value);
void setCurrentUser(Configuration conf, char * currentUser);