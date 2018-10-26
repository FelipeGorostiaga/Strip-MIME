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
#define TRUE 1
#define FALSE 0
#define MAXSPACES 10
#define CONTINUE   1
#define OK         0
#define NO_INPUT   1
#define TOO_LONG   2
#define MAX_STREAMS 64
#define ERROR_LENGTHM 5
#define BUFF_SIZE 1024

enum states {START,ERROR,WORD,VERSION,METRICS,GUION,HELP,QUIT,END,SPACE,LETTER};

void RemoveSpaces(char * source);
int getLine(char *prmpt, char *buff, size_t sz);
int readCommands();
void welcome();
void getProxyVersion();
void clearScreen();
void printErrorMessage(int errorCode);
int handleCommandProxy(char *buff, int cmd, int fd);
void authenticate(int fd);
void checkInput(int ret);
void cleanBuffer();
int connectSocket();
void initSettings();
int sendToProxy(int cmd, char * buffer, size_t size, int fd);
char * readFromProxy(int fd, int cmd);
void successMessage(int cmd);
int sendMetrics(char * buff, int fd);
void parseMetrics(char * response,int count);
void printMetric(char * metric, char * metricValue);
void tokenizeMetrics(char *buff);
int countDigits(char *str);
int hasErrorMessage(char * string);
void setToZero(char *buff, size_t size);
int isOK(char *s);
int isErrorMessage(char * string);

void testParseMetrics();
void testSendMetrics();
void test();
void test2();




