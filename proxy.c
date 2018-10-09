//
// Created by sswinnen on 07/09/18.
//

#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "proxy.h"
#define MAX_CLIENTS 30
#define MAX_PENDING 3
#define PORT 8080
#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 2048

int main(void) {

    int addrLen, max, descriptor, selectReturn, newSocket;
    int i = 0;
    int opt = TRUE;
    int clientSockets [MAX_CLIENTS];
    int passiveFd;
    char * successMessage = "client accepted successfully\0";
    char buffer[BUFFER_SIZE];
    struct sockaddr_in address;
    ssize_t readRet;
    fd_set readfds;

    /* Creating new socket */
    if ((passiveFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /*Binding socket to port */
    if (bind(passiveFd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("Binding socket to port failed");
        exit(EXIT_FAILURE);
    }
    /* Calling listen, necessary for passive socket to accept connections */
    if (listen(passiveFd, MAX_PENDING) < 0) {
        perror("Passive socket listen call failed");
        exit(EXIT_FAILURE);
    }
    if( setsockopt(passiveFd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    initializeClientArray(clientSockets);
    while(TRUE) {
        FD_ZERO (&readfds);
        FD_SET (passiveFd, &readfds);
        max = passiveFd;

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

        if (FD_ISSET(passiveFd, &readfds))
        {
            if ((newSocket = accept(passiveFd, (struct sockaddr *)&address, (socklen_t*)&addrLen))<0) {
                perror("Error accepting new client");
                exit(EXIT_FAILURE);
            }
            if( send(newSocket, successMessage, strlen(successMessage), 0) != strlen(successMessage) ) {
                perror("send");
            }
            for (i = 0; i < MAX_CLIENTS; i++) {
                if(clientSockets[i] == 0)
                {
                    clientSockets[i] = newSocket;
                    break;
                }
            }
        }
        for (i = 0; i < MAX_CLIENTS; i++) {
            descriptor = clientSockets[i];
            if (FD_ISSET( descriptor , &readfds)) {
                if ((readRet = read( descriptor , buffer, 1024)) == 0) {
                    getpeername(descriptor , (struct sockaddr*)&address , (socklen_t*)&addrLen);
                    close(descriptor);
                    clientSockets[i] = 0;
                }
                else {
                    buffer[readRet] = '\0';
                    /*HANDLE MESSAGE FROM CLIENT*/
                }
            }
        }
    }

}

void initializeClientArray(int * clients) {
    int i;

    for(i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = 0;
    }
}
