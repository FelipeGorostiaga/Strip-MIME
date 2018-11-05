#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define ALREADY_FOUND 2
#define RELEVANT_MIMES 4
#define RELEVANT_HEADERS 4
#define HNAME_BLOCK 50
#define HBODY_BLOCK 100
#define HBUF_BLOCK 100
#define HUTILS 0

typedef struct HeaderCDT * Header;
typedef struct HeaderCDT {
  char * name;
  int nameLength;
  char * body;
  int bodyLength;
} HeaderCDT;

typedef struct BoundaryCDT * Boundary;
typedef struct BoundaryCDT {
  char * bnd;
  int length;
} BoundaryCDT;

typedef struct ContentTypeParserCDT * ContentTypeParser;
typedef struct ContentTypeParserCDT {
  int contentTypeStatus;
  int previousContentTypeStatus;

  char potentialRelevantMime[4];
  char potentialMultipart;
  char potentialRfc822;
  int doubleQuoteCount;
  int boundaryNameIndex;
  int wspAndNewLines;

} ContentTypeParserCDT;

typedef struct HeaderParserCDT * HeaderParser;
typedef struct HeaderParserCDT {
  Header * relevantHeaders; // arreglo con headers que seran removidos si hay Content-Type censurable
  char * potentialRelevantHeader; // contiene TRUE si la data es incompleta (Content-Ty) o FALSE si no puede ser
  char isRfc822Mime; // si es Content-Type: message/rfc822
  char * hbuf; // header actual
  int hbufIndex;
  int lastTaskStatus;
  Boundary nextBoundary;

  ContentTypeParser ctp;
} HeaderParserCDT;
HeaderParser initializeHeaderParser();

typedef struct BodyParserCDT * BodyParser;
typedef struct BodyParserCDT {
  int bufferStartingPoint; // en que posicion del buffer global empieza el body
  int boundaryIndex; // buscando el boundary, cuantos caracteres van coincidiendo
  int hyphenCount;
  int isFinalPart; // si es la ultima parte antes de hacer pop(boundaries)
} BodyParserCDT;
BodyParser initializeBodyParser();

typedef enum {PARSING_HEADERS=1, DONE_PARSING_HEADERS, PARSING_BODY, DONE_PARSING_BODY, DONE_FINAL_BODY} status;

typedef enum {NEW_LINE=1,CR_HEADER,RELEVANT_HEADER_NAME,IRRELEVANT_HEADER,HEADER_BODY,CONTENT_TYPE_BODY} headerStatus;

typedef enum {NEW_LINE_CONTENT_TYPE=1,CR_MIME,MIME,SEPARATOR,COMMENT,BOUNDARY} contentTypeStatus;

/*typedef enum {NEW_LINE=1, SEPARATOR, HEADER_MATCHING,
 IRRELEVANT_HEADER, IRRELEVANT_HEADER_BODY, EMAIL_BODY, CR, INVALID} headerStatus;*/

typedef enum {NEW_LINE_BODY=1, CR_BODY, HYPHEN, IRRELEVANT_BODY, SEARCHING_HYPHEN,
  SEARCHING_BOUNDARY, BOUNDARY_POP_CHECK, WRITING_NO_CHECK} bodyStatus;

Header initializeHeader();
void resetHeaderParser(HeaderParser hp);
void resetBodyParser(BodyParser bp);
void resetBodyParserSameStartingPoint(BodyParser bp);
void freeHeaderContents(Header header);
void freeHeaderParser();
int matchesFound(HeaderParser);
void printHeader(Header);
void putHeaders(Header * headers);
