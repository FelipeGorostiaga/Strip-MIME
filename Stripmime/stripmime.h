#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "stack.h"
#include "headerAndBodyUtils.h"
#define COMPOSITE_MIMES 2
#define HYPHENS 2
#define CRLF 2
#define BUF_BLOCK 1000

char * relevantHeaderNames[RELEVANT_HEADERS] = {
  "Content-Type",
  "Content-Transfer-Encoding",
  //"Content-Disposition",
  "Content-Length", // Usage discouraged by RFC
  "Content-MD5"
};

typedef enum {MULTIPART=0,RFC822=1} composite;
char * compositeMimes[] = {
  "multipart/*",
  "message/rfc822"
};

typedef enum {FALSE=0, TRUE} bool;

typedef struct BufferCDT * Buffer;
typedef struct BufferCDT {
  char * buffer;
  ssize_t i;
  ssize_t bytesRead;
  bool doneReading;
} BufferCDT;
Buffer initializeBuffer();

ssize_t writeAndCheck(int fd, const void *buf, size_t count);

typedef enum {STDIN=0, STDOUT, STDERR} fileDescriptors;

typedef enum {SUCCESS=0, FAILURE, READ_FAILURE, WRITE_FAILURE} statusCodes;
char * errorMessages[] = {
  "OK",
  "Error occurred.",
  "Could not read the email message.",
  "Could not write to STDOUT."
};

typedef enum {NEW_LINE_POP3=1,CR_POP3,COMMAND_POP3} pop3StatusCodes;

typedef struct LevelCDT * Level;
typedef struct LevelCDT {
  int task; // tarea de mi maquina de estados de startConsuming
  int taskStatus; // tarea de maquina de estados secundaria como PARSE_HEADERS o PARSE_BODY
  bool isMultipart;
  bool mustBeCensored; // si encontre un MIME censurable

  HeaderParser headerParser;
  BodyParser bodyParser;
} LevelCDT;
Level initializeLevel();

typedef enum {CONTENT_TYPE=0,CONTENT_TRANSFER_ENCODING,
  CONTENT_LENGTH,CONTENT_MD5,ENCODING} relevantHeaders;

HeaderCDT replacementHeaderCDT = { 
  .name = "Content-Type:",
  .nameLength = 13,
  .body = " text/plain\r\n\r\n",
  .bodyLength = 15
};
Header replacementHeader = &replacementHeaderCDT;

char * boundaryName = "boundary";

int endWithFailure(const char * msg);

int caseInsensitiveStrncmp(const char * str1, const char * str2, size_t n);
