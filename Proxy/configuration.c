//
// Created by sswinnen on 13/10/18.
//


#include <inttypes.h>
#include <string.h>
#include "configuration.h"

char metric[20];

Configuration newConfiguration() {
    Configuration newConf = malloc(sizeof(configuration));
    newConf->help                   = "Mensaje de ayuda por defecto";
    newConf->pop3dir.s_addr         = INADDR_ANY;
    newConf->pop3dirFamily          = AF_INET;
    newConf->managementPort         = (uint16_t)9090;
    newConf->localPort              = (uint16_t)1110;
    newConf->originPort             = (uint16_t)110;
    newConf->version                = "0.0.0";
    newConf->concurrentConnections  = 0;
    newConf->bytesTransferred       = 0;
    newConf->totalAccesses          = 0;
    newConf->originServerString     = "";
    strcpy(newConf->errorFile,"/dev/null");
    strcpy(newConf->command, "cat");
    strcpy(newConf->replaceMessage,"Parte reemplazda.");
    setManagDir(newConf,"127.0.0.1");
    return newConf;
}

void deleteConfiguration(Configuration config) {
    free(config);
}

int portIsValid(const char * port) {
    int i = 0;

    while(port[i] != 0) {
        if(!isdigit(port[i])) {
            return FALSE;
        }
        i++;
    }
    return TRUE;
}

char * getErrorFile(Configuration conf) { return conf->errorFile; }
char * getHelp(Configuration conf) { return conf->help; }
struct in_addr getPop3dir(Configuration conf) { return conf->pop3dir; }
struct in6_addr getPop3dir6(Configuration conf) { return conf->pop3dir6; }
struct in_addr getManagDir(Configuration conf) { return conf->managDir; }
struct in6_addr getManagDir6(Configuration conf) { return conf->managDir6; }
char * getReplaceMessage(Configuration conf) { return conf->replaceMessage; }
char * getCensurableMediaTypes(Configuration conf) { return conf->censurableMediaTypes; }
uint16_t getManagementPort(Configuration conf) { return conf->managementPort; }
uint16_t getLocalPort(Configuration conf) { return conf->localPort; }
uint16_t getOriginPort(Configuration conf) { return conf->originPort; }
char * getCommand(Configuration conf) { return conf->command; }
char * getVersion(Configuration conf) { return conf->version; }
in_addr_t getOriginServer(Configuration conf) { return conf->originServer; }
int getOriginServerIsActive(Configuration conf) { return conf->originServerIsActive; }
char * getOriginServerString(Configuration conf) { return conf->originServerString; }
char * getCurrentUser(Configuration conf) { return conf->currentUser; }
int getPopDirFamily(Configuration conf) { return conf->pop3dirFamily; }
int getManagDirFamily(Configuration conf) { return conf->managDirFamily; }


int setErrorFile(Configuration conf, char * errorFile) {
    DIR* dir = opendir(errorFile);
    if (dir) {
        closedir(dir);
        strcpy(conf->errorFile, errorFile);
        return TRUE;
    }
    else if (ENOENT == errno) {
        fprintf(stderr,"File does not exist. Setting remains at default value\n");
    }
    return FALSE;
}
void setPop3dir(Configuration conf, char * pop3dir) {
    struct in_addr inaddr;
    struct in6_addr in6addr;

    if(inet_pton(AF_INET,pop3dir,&inaddr)) {
        conf->pop3dirFamily = AF_INET;
        conf->pop3dir = inaddr;
    } else if(inet_pton(AF_INET6,pop3dir,&in6addr)) {
        conf->pop3dirFamily = AF_INET6;
        conf->pop3dir6 = in6addr;
    } else {
        fprintf(stderr,"Invalid IP address for pop3 listening. Setting remains at default value\n");
    }

}
void setManagDir(Configuration conf, char * managDir) {
    struct in_addr inaddr;
    struct in6_addr in6addr;

    if(inet_pton(AF_INET,managDir,&inaddr)) {
        conf->managDirFamily = AF_INET;
        conf->managDir = inaddr;
    } else if(inet_pton(AF_INET6,managDir,&in6addr)) {
        conf->managDirFamily = AF_INET6;
        conf->managDir6 = in6addr;
    } else {
        fprintf(stderr,"Invalid IP address for pop3 listening. Setting remains at default value\n");
    }
}
int setReplaceMessage(Configuration conf, char * replaceMessage) {
    strcpy(conf->replaceMessage, replaceMessage);
    return TRUE;
}
int setCensurableMediaTypes(Configuration conf, char * censurableMediaTypes) {
    strcpy(conf->censurableMediaTypes, censurableMediaTypes);
    return TRUE;
}
void setManagementPort(Configuration conf, char * managementPort) {
    if(portIsValid(managementPort)) {
        uint16_t val = strToUint16(managementPort);
        conf->managementPort = val;
    }
    else {
        fprintf(stderr,"Invalid management port. Setting remains at default value\n");
    }
}
void setLocalPort(Configuration conf, char * localPort) {
    if(portIsValid(localPort)) {
        uint16_t val = strToUint16(localPort);
        conf->localPort = val;
    }
    else {
        fprintf(stderr,"Invalid local port. Setting remains at default value\n");
    }

}
void setOriginPort(Configuration conf, char * originPort) {
    if(portIsValid(originPort)) {
        uint16_t val = strToUint16(originPort);
        conf->originPort = val;
    }
    else {
        perror("Invalid origin port. Setting remains at default value\n");
    }
}
int setCommand(Configuration conf, char * command) {
    strcpy(conf->command,command);
    conf->command[strlen(command)] = 0;
    return TRUE;
}
void setOriginServer(Configuration conf, char * originServer) {
    struct addrinfo * result;
    int error;
    in_addr_t originServerAddress = 0;

    conf->originServerString = originServer;
    error = getaddrinfo(originServer, NULL, NULL, &result);
    if (error != 0) {
        if (error == EAI_SYSTEM) {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "Invalid origin server. error in getaddrinfo: %s\n", gai_strerror(error));
        }
        exit(EXIT_FAILURE);
    }
    else {
        if(inet_pton(result->ai_family,result->ai_addr->sa_data,&originServerAddress)) {
            conf->originServer = originServerAddress;
            conf->originServerIsActive = TRUE;
        } else {
            conf->originServerIsActive = FALSE;
        }
    }
}

void setOriginServerIsActive(Configuration conf,int value) {
    conf->originServerIsActive = FALSE;
}

void setCurrentUser(Configuration conf, char * currentUser) {
    strcpy(currentUser,conf->currentUser);
}

uint16_t strToUint16(const char * str) {
    uint16_t res;
    char * end;
    errno = 0;
    intmax_t val = strtoimax(str, &end, 10);
    if (errno == 34 || val < 0 || val > UINT16_MAX || end == str || *end != '\0') {
        return 0;
    }
    res = (uint16_t) val;
    return res;
}

char * getTotalAccesses(Configuration conf) {
    sprintf(metric, "%ld", conf->totalAccesses);
    return metric;
}

char * getBytesTransferred(Configuration conf) {
    sprintf(metric, "%ld", conf->bytesTransferred);
    return metric;
}

char * getConcurrentConnections(Configuration conf) {
    sprintf(metric, "%ld", conf->concurrentConnections);
    return metric;
}

void newAccess(Configuration conf) {
    conf->totalAccesses += 1;
}

void newConcurrentConnection(Configuration conf) {
    conf->concurrentConnections += 1;
}

void closedConcurrentConnection(Configuration conf) {
    conf->concurrentConnections -= 1;
}

void addBytesTransferred(Configuration conf, long bytes) {
    conf->bytesTransferred += bytes;
}