#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>


#define MAXSPACES 10
#define CONTINUE   1
#define OK         0
#define NO_INPUT   1
#define TOO_LONG   2
#define MAX_STREAMS 64


enum states {START,ERROR,WORD,VERSION,METRICS,GUION,HELP,QUIT,END,SPACE,LETTER};

int getLine(char *prmpt, char *buff, size_t sz);
int readCommands();
void welcome();
void getProxyVersion();
void clearScreen();
void printErrorMessage(int errorCode);
int handleCommandProxy(char *buff, int cmd, size_t  buffSize , char **metrics);
void createProtocolPackage(int command, char * buff, size_t buffsize);
void authenticate(int fd);
void checkInput(int ret);
void getMetricsParameters();
void cleanBuffer();
int connectSocket();
void initSettings();
void fillBuffer(char * sendbuff, char * buff, int cmd);
void getMetrics(char * buff, char ** params);