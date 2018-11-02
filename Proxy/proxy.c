//
// Created by sswinnen on 07/09/18.
//

#include "proxy.h"

Configuration config;
int popSocketFd, configSocketFd, genericOriginServer;
struct sockaddr_in popSocketAddress, configSocketAddress;
struct sockaddr_in6 psa6, csa6;
int clientSockets [MAX_CLIENTS];
int clientTypeArray [MAX_CLIENTS];
int originServerSockets [MAX_CLIENTS];
fd_set readfds;

int main(int argc, char * argv []) {
    fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL, 0) | O_NONBLOCK);
    config = newConfiguration();
    if(parseArguments(config, argc, argv) == END) {
        return 0;
    }
    popSocketFd = createPassiveSocket(&popSocketAddress,&psa6,getPop3dir6(config),getPop3dir(config),getLocalPort(config),getPopDirFamily(config), IPPROTO_TCP);
    configSocketFd = createPassiveSocket(&configSocketAddress,&csa6,getManagDir6(config),getManagDir(config),getManagementPort(config), getManagDirFamily(config),IPPROTO_SCTP);
    genericOriginServer = socketToOriginServer(getOriginServer(config));
    firstReadToGeneric();
    makeNonBlocking(genericOriginServer);
    startFilter();
    selectLoop();
    return 0;
}

int createPassiveSocket(struct sockaddr_in * address, struct sockaddr_in6 * address6,
                        struct in6_addr addr6, struct in_addr addr, uint16_t port,int family, int transportProtocol) {
    int opt = TRUE;
    int fd;

    printf("Port for %s: %d\n",transportProtocol == IPPROTO_TCP ? "local" : "management", port);
    if ((fd = socket(family, SOCK_STREAM, transportProtocol)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    if(family == AF_INET) {
        address->sin_family         = AF_INET;
        address->sin_addr.s_addr    = addr.s_addr;
        address->sin_port           = htons(port);
        if (bind(fd, (struct sockaddr *)address, sizeof(*address))<0) {
            perror("Binding socket to port failed");
            exit(EXIT_FAILURE);
        }
    } else {
        address6->sin6_family       = AF_INET6;
        address6->sin6_addr.__in6_u = addr6.__in6_u;
        address6->sin6_port         = htons(port);
        if (bind(fd, (struct sockaddr *)address6, sizeof(*address6))<0) {
            perror("Binding socket to port failed");
            exit(EXIT_FAILURE);
        }
    }


    if (listen(fd, MAX_PENDING) < 0) {
        perror("socket listen call failed");
        exit(EXIT_FAILURE);
    }
    if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int socketToOriginServer(in_addr_t address) {
    int socketfd;
    struct sockaddr_in servaddr;

    socketfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof servaddr);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(getOriginPort(config));
    servaddr.sin_addr.s_addr   = address;


    if(connect(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Connection failed\n");
        config->originServerIsActive = FALSE;
        return -1;
    } else {
        config->originServerIsActive = TRUE;
    }

    return socketfd;
}

void selectLoop() {
    int selectReturn, max, descriptor, i;
    while(TRUE) {
        FD_ZERO (&readfds);
        FD_SET(popSocketFd, &readfds);
        FD_SET(configSocketFd, &readfds);
        FD_SET(genericOriginServer, & readfds);
        max = configSocketFd > popSocketFd ? configSocketFd : popSocketFd;
        max = genericOriginServer > max ? genericOriginServer : max;
        for ( i = 0 ; i < MAX_CLIENTS ; i++) {
            descriptor = clientSockets[i];
            if(descriptor > 0) {
                FD_SET(descriptor, &readfds);
            }
            if(descriptor > max) {
                max = descriptor;
            }
        }
        selectReturn = select( max + 1 , &readfds , NULL , NULL , NULL);
        if ((selectReturn < 0) && (errno != EINTR)) {
            printf("select error");
        }
        updateOriginOpenness();
        checkForNewClients(configSocketFd,ADMIN);
        checkForNewClients(popSocketFd,CLIENT);
        readFromClients();
    }
}

void checkForNewClients(int socket, int clientType) {

    int  newSocket, addrLen, i;
    char buffer[BUFFER_SIZE];

    if (FD_ISSET(socket, &readfds)) {

        printf("HAY UN CLIENTE NUEVO!\n");
        if ((newSocket = accept(socket, (struct sockaddr *)&popSocketAddress, (socklen_t*)&addrLen))<0) {
            perror("Error accepting new client");
            exit(EXIT_FAILURE);
        }
        if(clientType == CLIENT && !getOriginServerIsActive(config)) {
            write(newSocket,"-ERR Connection refused\r\n", strlen("-ERR Connection refused\r\n"));
            close(newSocket);
            return;
        }
        for (i = 0; i < MAX_CLIENTS; i++) {
            if(clientSockets[i] == 0) {
                clientSockets[i] = newSocket;
                clientTypeArray[i] = clientType;
                originServerSockets[i] = socketToOriginServer(getOriginServer(config));
                break;
            }
        }
        printf("YA TE ACEPTE!\n");
        if(clientType == CLIENT) {
            printf("SOS POP3, AHI TE MANDO LA BIENVENIDA!\n");
            if( write(newSocket,buffer,(size_t )read(originServerSockets[i],buffer,BUFFER_SIZE)) == -1) {
                perror("Error at send call");
            }
            printf("YA TE LA MANDE!\n");
        } else {
            printf("New admin accepted\n");
        }
        makeNonBlocking(newSocket);
    }
}

void readFromClients() {
    int i, descriptor, retVal;
    char env[BUFFER_SIZE] = {0};
    char * envVariables [5];
    struct attendReturningFields rf;

    for (i = 0; i < MAX_CLIENTS; i++) {
        descriptor = clientSockets[i];
        if (FD_ISSET( descriptor , &readfds)) {
            if(clientTypeArray[i] == ADMIN) {
                retVal = parseConfig(descriptor, config);
                if(retVal == CLOSED_SOCKET) {
                    clientTypeArray[i] = clientSockets[i] = 0;
                }
            }
            else if(clientTypeArray[i] == CLIENT){
                if(!getOriginServerIsActive(config)) {
                    write(descriptor,"-ERR Connection refused\r\n", strlen("-ERR Connection refused\r\n"));
                    close(descriptor);
                    close(originServerSockets[i]);
                    clientTypeArray[i] = clientSockets[i] = originServerSockets[i] = 0;
                    return;
                }
                newAccess(config);
                newConcurrentConnection(config);
                strcpy(env,"FILTER_MEDIAS=");
                strcpy(env+strlen("FILTER_MEDIAS="),getCensurableMediaTypes(config));
                envVariables[0] = env;
                strcpy(env,"FILTER_MSG=");
                strcpy(env+strlen("FILTER_MSG="),getReplaceMessage(config));
                envVariables[1] = env;
                strcpy(env,"POP3FILTER_VERSION=");
                strcpy(env+strlen("POP3FILTER_VERSION="),getVersion(config));
                envVariables[2] = env;
                strcpy(env,"POP3_USERNAME=");
                strcpy(env+strlen("POP3_USERNAME="),getCurrentUser(config));
                envVariables[3] = env;
                strcpy(env,"POP3_SERVER=");
                strcpy(env+strlen("POP3_SERVER="),getOriginServerString(config));
                envVariables[4] = env;
                rf = attendClient(descriptor,originServerSockets[i], envVariables);
                if(rf.closeConnectionFlag == TRUE) {
                    closedConcurrentConnection(config);
                    close(descriptor);
                    close(originServerSockets[i]);
                    clientTypeArray[i] = clientSockets[i] = originServerSockets[i] = 0;
                }
                addBytesTransferred(config,rf.bytesTransferred);
            }
        }
    }
}

void makeNonBlocking(int fd) {
    int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1){
        fprintf(stderr,"Error in fcntl");
    }
}

void updateOriginOpenness() {
    char c;

    if(getOriginServerIsActive(config) == TRUE && FD_ISSET(genericOriginServer,&readfds)) {
        if(read(genericOriginServer,&c,1) == 0) {
            setOriginServerIsActive(config,FALSE);
        }
    }
}

void firstReadToGeneric() {
    char buff [BUFF_SIZE];

    read(genericOriginServer,buff,BUFF_SIZE);
}