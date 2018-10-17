//
// Created by sswinnen on 13/10/18.
//


#include <inttypes.h>
#include "configuration.h"


Configuration newConfiguration() {
    Configuration newConf = malloc(sizeof(configuration));
    newConf->errorFile              = "/dev/null";
    newConf->help                   = "Mensaje de ayuda por defecto";
    newConf->pop3dir                = INADDR_ANY;
    newConf->replaceMessage         = "Parte reemplazda.";
    newConf->censurableMediaTypes   = "";
    newConf->managementPort         = (uint16_t)9090;
    newConf->localPort              = (uint16_t)1110;
    newConf->originPort             = (uint16_t)110;
    newConf->command                = "";
    newConf->version                = "0.0.0";
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
in_addr_t getPop3dir(Configuration conf) { return conf->pop3dir; }
in_addr_t getManagDir(Configuration conf) { return conf->managDir; }
char * getReplaceMessage(Configuration conf) { return conf->replaceMessage; }
char * getCensurableMediaTypes(Configuration conf) { return conf->censurableMediaTypes; }
uint16_t getManagementPort(Configuration conf) { return conf->managementPort; }
uint16_t getLocalPort(Configuration conf) { return conf->localPort; }
uint16_t getOriginPort(Configuration conf) { return conf->originPort; }
char * getCommand(Configuration conf) { return conf->command; }
char * getVersion(Configuration conf) { return conf->version; }
in_addr_t getOriginServer(Configuration conf) { return conf->originServer; }


void setErrorFile(Configuration conf, char * errorFile) {
    DIR* dir = opendir(errorFile);
    if (dir) {
        closedir(dir);
        conf->errorFile = errorFile;
    }
    else if (ENOENT == errno) {
        fprintf(stderr,"File does not exist. Setting remains at default value\n");
    }
}
void setPop3dir(Configuration conf, char * pop3dir) {
    in_addr_t intDir = 0;
    if(inet_pton(AF_INET,pop3dir,&intDir)) {
        conf->pop3dir = intDir;
    } else {
        fprintf(stderr,"Invalid IP address for pop3 listening. Setting remains at default value\n");
    }

}
void setManagDir(Configuration conf, char * managDir) {
    in_addr_t intDir = 0;
    if(inet_pton(AF_INET,managDir,&intDir)) {
        conf->managDir = intDir;
    } else {
        fprintf(stderr,"Invalid IP address for management. Setting remains at default value\n");
    }
}
void setReplaceMessage(Configuration conf, char * replaceMessage) {
    conf->replaceMessage = replaceMessage;
}
void setCensurableMediaTypes(Configuration conf, char * censurableMediaTypes) {
    conf->censurableMediaTypes = censurableMediaTypes;
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
void setCommand(Configuration conf, char * command) {
    conf->command = command;
}
void setOriginServer(Configuration conf, char * originServer) {
    struct addrinfo * result;
    int error;
    in_addr_t originServerAddress = 0;

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
        inet_pton(result->ai_family,result->ai_addr->sa_data,&originServerAddress);
        conf->originServer = originServerAddress;
    }
}

uint16_t strToUint16(const char * str) {
    uint16_t res;
    char * end;
    errno = 0;
    intmax_t val = strtoimax(str, &end, 10);
    if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0') {
        return 0;
    }
    res = (uint16_t) val;
    return res;
}