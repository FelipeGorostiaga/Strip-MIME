#include "headerAndBodyUtils.h"
#define STDOUT 1

static void setToTrue(char * arr, int size);

HeaderParser initializeHeaderParser() {
  HeaderParser hp = malloc(sizeof(HeaderParserCDT));
  hp->relevantHeaders = NULL;
  hp->potentialRelevantHeader = NULL;
  hp->hbuf = NULL;
  hp->nextBoundary = NULL;
  hp->ctp = NULL;
  resetHeaderParser(hp);
  return hp;
}

void freeHeaderParser(HeaderParser hp) {
  int index;
  for(index = 0; index < RELEVANT_HEADERS; index++) {
    if(hp->relevantHeaders[index] != NULL) {
      freeHeaderContents(hp->relevantHeaders[index]);
      free(hp->relevantHeaders[index]);
    }
  }
  free(hp->potentialRelevantHeader);
  free(hp->hbuf);
  if(hp->nextBoundary != NULL) {
    free(hp->nextBoundary->bnd);
    free(hp->nextBoundary);
  }
  //free(hp->ctp->potentialRelevantMime);
  free(hp->ctp);
}

void freeHeaderContents(Header header) {
  free(header->name);
  free(header->body);
}

Header initializeHeader() {
  Header h = malloc(sizeof(HeaderCDT));
  h->name = malloc(HNAME_BLOCK);
  h->body = malloc(HBODY_BLOCK);
  h->nameLength = h->bodyLength = 0;
  return h;
}

void resetHeaderParser(HeaderParser hp) {
  if(hp->relevantHeaders == NULL)
    hp->relevantHeaders = calloc(RELEVANT_HEADERS, sizeof(Header));
  if(hp->potentialRelevantHeader == NULL)
    hp->potentialRelevantHeader = malloc(RELEVANT_HEADERS * sizeof(char));
  setToTrue(hp->potentialRelevantHeader, RELEVANT_HEADERS);
  hp->isRfc822Mime = 0;
  if(hp->hbuf == NULL)
    hp->hbuf = malloc(HBUF_BLOCK);
  hp->hbufIndex = 0; // valgrind
  hp->nextBoundary = malloc(sizeof(Boundary));
  hp->nextBoundary->bnd = malloc(HBUF_BLOCK);
  hp->nextBoundary->length = 0; // valgrind invalid read size 4
//char arr[400] = {0};
//sprintf(arr, "Puntero del boundary: %p\n", hp->nextBoundary);
//write(STDOUT, arr, strlen(arr));
  if(hp->ctp == NULL)
    hp->ctp = malloc(sizeof(ContentTypeParserCDT));
  int index;
  for(index = 0; index < RELEVANT_MIMES; index++)
    hp->ctp->potentialRelevantMime[index] = 1;
  hp->ctp->boundaryNameIndex = hp->ctp->doubleQuoteCount = hp->ctp->wspAndNewLines = 0;
  hp->ctp->potentialMultipart = hp->ctp->potentialRfc822 = 1;
}

static void setToTrue(char * arr, int size) {
  int index;
  for(index = 0; index < size; index++)
    arr[index] = 1;
}

void printHeader(Header h) {
  write(STDOUT, h->name, h->nameLength);
  write(STDOUT, h->body, h->bodyLength);
}

void putHeaders(Header * headers) {
  int index;
  for(index = 0; index < RELEVANT_HEADERS; index++) {
    if(headers[index] != NULL)
      printHeader(headers[index]);
  }
}

/*int findMatchingRelevantHeader(HeaderParser hp) {
	int index;
	for(index = 0; index < RELEVANT_HEADERS; index++) {
		if(hp->potentialRelevantHeader[index] == 1
			&& hp->hbufIndex == strlen(relevantHeaderNames[index]) + 1)
			return index;
	}
	return -1;
}*/

int matchesFound(HeaderParser hp) {
	char * arr = hp->potentialRelevantHeader;
	int index;
	for(index = 0; index < RELEVANT_HEADERS; index++) {
		if(arr[index] != 0) {
			return index;
		}
	}
	return -1;
}

BodyParser initializeBodyParser() {
  BodyParser bp = malloc(sizeof(BodyParserCDT));
  resetBodyParser(bp);
  return bp;
}

void resetBodyParser(BodyParser bp) {
  bp->bufferStartingPoint = bp->boundaryIndex = bp->hyphenCount = bp->isFinalPart = 0;
}

void resetBodyParserSameStartingPoint(BodyParser bp) {
  bp->boundaryIndex = bp->hyphenCount = bp->isFinalPart = 0;
}
