//
// Created by sswinnen on 17/10/18.
//

#include <unistd.h>
#include <stdio.h>
#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 256
#define BUFFER_END 15


void attendClient(int clientSockFd, int originServerSock);

/*Client attention when origin server supports pipelining*/
void pipeliningMode();

/*Client attention when origin server does not support pipelining*/
void noPipeliningMode();

/*Separates indiviudual commands and sends them to server. Returns the index of start of the last unfinished command*/
int parseChunk(char * buffer, ssize_t bytesRead);

/*When \r\n is found, the end of the command is written and no more con
 content is received from client until the end of the server response arrives */
void writeCommandEnd(char * buffer, size_t len);

/*Performs simultaneous write and reads to filter*/
void writeAndReadFilter();

/*Reads from client and forwards to origin server */
int readFromClient();

/*Reads from origin server returning TRUE if reading has finished FALSE if not */
int readFromOrigin();

/*Reads from filter returning TRUE if reading has finished FALSE if not */
int readFromFilter();

/*Excecutes filter and opens pipes*/
void startFilter();

/*Checks with origin server if pipelining is supported*/
int pipeliningSupport(int originServer);

/*Returns TRUE if str contains "PIPELINING"*/
int findPipelining(char * str, ssize_t size);

/*Looks for \r\n and returns the position where it was found. If no \r\n was found returns 0*/
size_t commandEnd(const char * buffer, ssize_t size);