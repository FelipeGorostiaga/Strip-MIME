#include "headerAndBodyUtils.h"
#define STDOUT 1

static void setToTrue(char * arr, int size);
static void setPotentialRelevantMimesToTrue(ContentTypeParser ctp, int relevantMimes);

HeaderParser initializeHeaderParser(int relevantMimes) {
  HeaderParser hp = malloc(sizeof(HeaderParserCDT));
  hp->relevantHeaders = NULL;
  hp->potentialRelevantHeader = NULL;
  hp->hbuf = NULL;
  hp->nextBoundary = NULL;
  hp->ctp = NULL;
  resetHeaderParser(hp, relevantMimes);
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
  free(hp->ctp->potentialRelevantMime);
  free(hp->ctp);
}

void freeHeaderContents(Header header) {
  if(header->name != NULL)
    free(header->name);
  if(header->body != NULL)
    free(header->body);
}

Header initializeHeader() {
  Header h = malloc(sizeof(HeaderCDT));
  h->name = malloc(HNAME_BLOCK);
  h->body = malloc(HBODY_BLOCK);
  h->nameLength = h->bodyLength = 0;
  return h;
}

void resetHeaderParser(HeaderParser hp, int relevantMimes) {
  if(hp->relevantHeaders == NULL)
    hp->relevantHeaders = calloc(RELEVANT_HEADERS, sizeof(Header));
  else {
    int index;
    for(index = 0; index < RELEVANT_HEADERS; index++) {
      if(hp->relevantHeaders[index] != NULL) {
        freeHeaderContents(hp->relevantHeaders[index]);
        hp->relevantHeaders[index]->name = NULL;
        hp->relevantHeaders[index]->body = NULL;
      }
    }
  }
  if(hp->potentialRelevantHeader == NULL)
    hp->potentialRelevantHeader = malloc(RELEVANT_HEADERS * sizeof(char));
  setToTrue(hp->potentialRelevantHeader, RELEVANT_HEADERS);
  hp->isRfc822Mime = 0;
  if(hp->hbuf == NULL)
    hp->hbuf = malloc(HBUF_BLOCK);
  hp->hbufIndex = 0;
  hp->nextBoundary = malloc(sizeof(Boundary));
  hp->nextBoundary->bnd = malloc(HBUF_BLOCK);
  hp->nextBoundary->length = 0; // valgrind invalid read size 4 (?)
  if(hp->ctp == NULL) {
    hp->ctp = malloc(sizeof(ContentTypeParserCDT));
    hp->ctp->potentialRelevantMime = NULL;
  }
  if(hp->ctp->potentialRelevantMime == NULL) {
    hp->ctp->potentialRelevantMime = malloc(relevantMimes * sizeof(char));
  }
  setPotentialRelevantMimesToTrue(hp->ctp, relevantMimes);
  hp->ctp->boundaryNameIndex = hp->ctp->doubleQuoteCount = hp->ctp->wspAndNewLines = 0;
  hp->ctp->potentialMultipart = hp->ctp->potentialRfc822 = 1;
}

static void setPotentialRelevantMimesToTrue(ContentTypeParser ctp, int relevantMimes) {
  int index;
  for(index = 0; index < relevantMimes; index++)
    ctp->potentialRelevantMime[index] = 1;
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
