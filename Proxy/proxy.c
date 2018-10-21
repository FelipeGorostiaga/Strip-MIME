//
// Created by sswinnen on 07/09/18.
//

#include <fcntl.h>
#include "proxy.h"

static Configuration config;
static int popSocketFd, configSocketFd, originServerFd;
static struct sockaddr_in popSocketAddress, configSocketAddress;
static int clientSockets [MAX_CLIENTS]     = {0};
static int clientTypeArray [MAX_CLIENTS]   = {0};
static fd_set readfds;

int main(int argc, char * argv []) {
    config = newConfiguration();
    if(parseArgs(config, argc, argv) == END) {
        return 0;
    }
    popSocketFd = createPassiveSocket(&popSocketAddress,getPop3dir(config),getLocalPort(config), IPPROTO_TCP);
    configSocketFd = createPassiveSocket(&configSocketAddress,getManagDir(config),getManagementPort(config),IPPROTO_SCTP);
    originServerFd = socketToOriginServer(getOriginServer(config));
    selectLoop();
    return 0;
}

int createPassiveSocket(struct sockaddr_in * address,in_addr_t direction, uint16_t port, int transportProtocol) {
    int opt = TRUE;
    int fd;

    printf("Port for %s: %d\n",transportProtocol == IPPROTO_TCP ? "local" : "management", port);
    if ((fd = socket(AF_INET, SOCK_STREAM, transportProtocol)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    address->sin_family         = AF_INET;
    address->sin_addr.s_addr    = direction;
    address->sin_port           = htons(port);
    if (bind(fd, (struct sockaddr *)address, sizeof(*address))<0) {
        perror("Binding socket to port failed");
        exit(EXIT_FAILURE);
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
        return -1;
    }

    return socketfd;
}

void selectLoop() {
    int selectReturn, max, descriptor, i;
    while(TRUE) {
        FD_ZERO (&readfds);
        FD_SET(popSocketFd, &readfds);
        FD_SET(configSocketFd, &readfds);
        FD_SET(originServerFd, &readfds);
        max = configSocketFd > popSocketFd ? configSocketFd : popSocketFd;
        max = max > originServerFd ? max : originServerFd;
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
        checkForNewClients(popSocketFd,CLIENT);
        checkForNewClients(configSocketFd,ADMIN);
        readFromClients();
    }
}

void checkForNewClients(int socket, int clientType) {

    int  newSocket, addrLen, i;
    char * successMessage = "Success\n";

    if (FD_ISSET(socket, &readfds)) {
        if ((newSocket = accept(popSocketFd, (struct sockaddr *)&popSocketAddress, (socklen_t*)&addrLen))<0) {
            perror("Error accepting new client");
            exit(EXIT_FAILURE);
        }
        if( send(newSocket, successMessage, strlen(successMessage), 0) != strlen(successMessage) ) {
            perror("Error at send call");
        }
        printf("Client accepted successfully\n");
        for (i = 0; i < MAX_CLIENTS; i++) {
            if(clientSockets[i] == 0) {
                clientSockets[i] = newSocket;
                clientTypeArray[i] = clientType;
                break;
            }
        }
        makeNonBlocking(newSocket);
    }
}

void readFromClients() {
    int i, descriptor, retVal;

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
                attendClient(descriptor,originServerFd);
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