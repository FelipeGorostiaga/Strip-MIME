//
// Created by sswinnen on 17/10/18.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0
#define BUFF_SIZE 1024
#define BUFFER_END 15
#define NOT_SUPPORTED (-1)
#define CONTINUE 0
#define QUIT 2
#define QUITCOMMAND 3

typedef struct attendReturningFields {
    int closeConnectionFlag;
    long bytesTransferred;
    char writeBuffer [BUFF_SIZE];
    ssize_t size;
    int socket;
    int retValue;
} arf;


struct attendReturningFields attendClient(int clientSockFd, int originServerSock);

/*Client attention when origin server supports pipelining*/
void pipeliningMode(int client, int originServer);

/*Reads from client and forwards to origin server */
int readFromClient(int client,int originServer);

/*Reads from origin server returning TRUE if reading has finished FALSE if not */
int readFromOrigin(int originServer,int filterInput);

/*Reads from filter returning TRUE if reading has finished FALSE if not */
int readFromFilter(int filterOutput, int clientFd);

/*Excecutes filter and opens pipes*/
int * startFilter(char * command, char * envVariables [5]);

/*Checks with origin server if pipelining is supported*/
int pipeliningSupport(int originServer);

/*Returns TRUE if str contains "PIPELINING"*/
int findPipelining(char * str, ssize_t size);

/*Writes current command in logs*/
void logAccess(char * buffer, size_t cmdStart);

