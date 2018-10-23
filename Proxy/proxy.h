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
#include <fcntl.h>
#include "configParser.h"
#include "pop3parser.h"
#define MAX_CLIENTS 30
#define MAX_PENDING 3
#define CLIENT 1
#define ADMIN 2

/* Creates passive socket */
int createPassiveSocket(struct sockaddr_in * address,in_addr_t direction, uint16_t port, int transportProtocol);

/*Creates socket to origin server*/
int socketToOriginServer(in_addr_t address);

/* Iterates indefinitely performing calls to select */
void selectLoop();

/* Checks if passive sockets have new connection petitions */
void checkForNewClients(int socket, int clientType);

/* Iterates over clients and reads if necessary */
void readFromClients();

/*Makes the file descriptor fd non blocking*/
void makeNonBlocking(int fd);
