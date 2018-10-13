//
// Created by sswinnen on 13/10/18.
//


#include "configuration.h"

Configuration newConfiguration() {
    Configuration newConf = malloc(sizeof(configuration));
    newConf->errorFile              = "/dev/null";
    newConf->help                   = "Mensaje de ayuda por defecto";
    newConf->pop3dir                = "todas";
    newConf->managDir               = "loopback";
    newConf->replaceMessage         = "Parte reemplazda.";
    newConf->censurableMediaTypes   = "";
    newConf->managementPort         = "9090";
    newConf->localPort              = "1110";
    newConf->originPort             = "110";
    newConf->command                = "";
    newConf->version                = "0.0.0";
    return newConf;
};

void deleteConfiguration(Configuration config) {
    free(config);
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


void setErrorFile(Configuration conf, char * errorFile) { conf->errorFile = errorFile; }
void setPop3dir(Configuration conf, char * pop3dir) { conf->pop3dir = pop3dir; }
void setManagDir(Configuration conf, char * managDir) { conf->managDir = managDir; }
void setReplaceMessage(Configuration conf, char * replaceMessage) { conf->replaceMessage = replaceMessage; }
void setCensurableMediaTypes(Configuration conf, char * censurableMediaTypes) { conf->censurableMediaTypes = censurableMediaTypes; }
void setManagementPort(Configuration conf, char * managementPort) { conf->managementPort = managementPort; }
void setLocalPort(Configuration conf, char * localPort) { conf->localPort = localPort; }
void setOriginPort(Configuration conf, char * originPort) { conf->originPort = originPort; }
void setCommand(Configuration conf, char * command) { conf->command = command; }
void setOriginServer(Configuration conf, char * originServer) { conf->originServer = originServer; }








