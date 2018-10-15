//
// Created by sswinnen on 13/10/18.
//


#include "configuration.h"


Configuration newConfiguration() {
    Configuration newConf = malloc(sizeof(configuration));
    newConf->errorFile              = "/dev/null";
    newConf->help                   = "Mensaje de ayuda por defecto";
    newConf->pop3dir                = "todas";
    newConf->managDir               = "127.0.0.1";
    newConf->replaceMessage         = "Parte reemplazda.";
    newConf->censurableMediaTypes   = "";
    newConf->managementPort         = "9090";
    newConf->localPort              = "1110";
    newConf->originPort             = "110";
    newConf->command                = "";
    newConf->version                = "0.0.0";
    newConf->originServer           = "";
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
char * getPop3dir(Configuration conf) { return conf->pop3dir; }
char * getManagDir(Configuration conf) { return conf->managDir; }
char * getReplaceMessage(Configuration conf) { return conf->replaceMessage; }
char * getCensurableMediaTypes(Configuration conf) { return conf->censurableMediaTypes; }
char * getManagementPort(Configuration conf) { return conf->managementPort; }
char * getLocalPort(Configuration conf) { return conf->localPort; }
char * getOriginPort(Configuration conf) { return conf->originPort; }
char * getCommand(Configuration conf) { return conf->command; }
char * getVersion(Configuration conf) { return conf->version; }
char * getOriginServer(Configuration conf) { return conf->originServer; }
int getLocalPortAsInt(Configuration conf) { return atoi(conf->localPort); }
int getOriginPortAsInt(Configuration conf) { return atoi(conf->originPort); }
int getManagementPortAsInt(Configuration conf) { return atoi(conf->managementPort); }


void setErrorFile(Configuration conf, char * errorFile) {
    DIR* dir = opendir("mydir");
    if (dir) {
        closedir(dir);
        conf->errorFile = errorFile;
    }
    else if (ENOENT == errno) {
        perror("File does not exist. Setting remains at default value\n");
    }
}
void setPop3dir(Configuration conf, char * pop3dir) {
    //TODO antes de setear la configuracion intentar asignarlo en el proxy y se falla devolver mensaje de error
    conf->pop3dir = pop3dir;
}
void setManagDir(Configuration conf, char * managDir) {
    //TODO antes de setear la configuracion intentar asignarlo en el proxy y se falla devolver mensaje de error
    conf->managDir = managDir;
}
void setReplaceMessage(Configuration conf, char * replaceMessage) {
    conf->replaceMessage = replaceMessage;
}
void setCensurableMediaTypes(Configuration conf, char * censurableMediaTypes) {
    conf->censurableMediaTypes = censurableMediaTypes;
}
void setManagementPort(Configuration conf, char * managementPort) {
    if(portIsValid(managementPort)) {
        conf->localPort = managementPort;
    }
    else {
        perror("Invalid management port. Setting remains at default value\n");
    }
    conf->managementPort = managementPort;
}
void setLocalPort(Configuration conf, char * localPort) {
    if(portIsValid(localPort)) {
        conf->localPort = localPort;
    }
    else {
        perror("Invalid local port. Setting remains at default value\n");
    }

}
void setOriginPort(Configuration conf, char * originPort) {
    if(portIsValid(originPort)) {
        conf->originPort = originPort;
    }
    else {
        perror("Invalid origin port. Setting remains at default value\n");
    }
}
void setCommand(Configuration conf, char * command) {
    conf->command = command;
}
void setOriginServer(Configuration conf, char * originServer) {
    conf->originServer = originServer;
}








