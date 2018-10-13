//
// Created by sswinnen on 07/09/18.
//

#include "proxy.h"
#include "shellArgsParser.h"

static Configuration config;
static int popSocketFd, configSocketFd;
static struct sockaddr_in popSocketAddress, configSocketAddress;
static int clientSockets [MAX_CLIENTS]     = {0};
static int clientTypeArray [MAX_CLIENTS]   = {0};
static fd_set readfds;

int main(int argc, char * argv []) {
    config = newConfiguration();
    parseArgs(config, argc, argv);
    popSocketFd = createPassiveSocket(&popSocketAddress,TCP_SOCKET_PORT, 0);
    configSocketFd = createPassiveSocket(&configSocketAddress,SCTP_SOCKET_PORT,IPPROTO_SCTP);
    selectLoop();
    return 0;
}

int createPassiveSocket(struct sockaddr_in * address, uint16_t port, int transportProtocol) {
    int opt = TRUE;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, transportProtocol)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    address->sin_family         = AF_INET;
    address->sin_addr.s_addr    = INADDR_ANY;
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

void selectLoop() {
    int selectReturn, max, descriptor, i;

    while(TRUE) {
        FD_ZERO (&readfds);
        FD_SET (popSocketFd, &readfds);
        FD_SET(configSocketFd, &readfds);
        max = configSocketFd > popSocketFd ? configSocketFd : popSocketFd;

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
        if ((selectReturn < 0) && (errno!=EINTR)) {
            printf("select error");
        }
        checkForNewClients(popSocketFd,MUA);
        checkForNewClients(configSocketFd,ADMIN);
        readFromClients();
    }
}

void checkForNewClients(int socket, int clientType) {

    int  newSocket, addrLen, i;
    char * successMessage = "client accepted successfully\0";

    if (FD_ISSET(socket, &readfds))
    {
        if ((newSocket = accept(popSocketFd, (struct sockaddr *)&popSocketAddress, (socklen_t*)&addrLen))<0) {
            perror("Error accepting new client");
            exit(EXIT_FAILURE);
        }
        if( send(newSocket, successMessage, strlen(successMessage), 0) != strlen(successMessage) ) {
            perror("Error at send call");
        }
        for (i = 0; i < MAX_CLIENTS; i++) {
            if(clientSockets[i] == 0)
            {
                clientSockets[i] = newSocket;
                clientTypeArray[i] = clientType;
                break;
            }
        }
    }
}

void readFromClients() {
    int i, descriptor, retVal;

    for (i = 0; i < MAX_CLIENTS; i++) {
        descriptor = clientSockets[i];
        if (FD_ISSET( descriptor , &readfds)) {
            if(clientTypeArray[i] == ADMIN) {
                retVal = parseConfig(descriptor);
                if(retVal == CLOSED_SOCKET) {
                    clientTypeArray[i] = clientSockets[i] = 0;
                }
            } else {
                //TODO pop3 parser
            }
        }
    }
}



