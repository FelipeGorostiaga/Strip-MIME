#include "stripmime.h"

static void getEnvironmentVariables();
static void splitMimes(char * mimes);
static void startConsuming(Level current);
static void feedMoreBytes(Buffer buffer);
static void consumeRemainingBytes();
static void emptyBoundaries(Stack stack);
static void resetLevel(Level level);
static void freeResources(Level startingLevel, Buffer buffer, Stack boundaries);
static void freeLevelResources(Level level);
static void freeMimes();
static void parseHeader(Level current, char c);
static void parseHeaderParams(Level current, char c);
static void injectReplacementHeaderAndBody();
static void parseBody(Level current, char c, Boundary boundary);
static void writeBoundary(bool isFinalPart, Boundary currentBoundary);
static void setToTrueIfNotFound(char * potentialRelevantHeader, int relevantHeaders);
static bool checkIsRelevantHeader(Level current, char c);
static void addCharToHeaderBuffer(HeaderParser hp, char c);
static bool cmp(const char * str, int position, char c);
static bool cmpMime(const char * str, int position, char c);
static bool anyFinishedMatch(HeaderParser hp);
static void addCharToHeaderBody(HeaderParser hp, int position, char c);
static bool checkIsRelevantMime(char * potentialRelevantMime, int position, char c);
static bool checkCompositeMime(int position, char c, int compositeMime, char potential);
static bool anyCensoredMatches(const char * potentialRelevantMime, int position);
static bool isComposite(char, int, int);
static void addCharToBoundary(HeaderParser hp, char c);
static void parsePop3(Level current, char c);
void resetLevelValues(Level);

Stack boundaries = NULL; // stack containing non-censored multipart boundaries
Buffer buffer = NULL; // universal buffer
char * replacementBody = "This is the default replacement message. \
 Your message has been censored.";
int relevantMimes = COMPOSITE_MIMES;
char ** censoredMimes = NULL;

int pop3StartingPoint = 0;

int main(void) {
  getEnvironmentVariables();
  write(STDOUT, "setee vars", strlen("setee vars"));
  boundaries = newStack();
  buffer = initializeBuffer();
  Level startingLevel = initializeLevel();
  startConsuming(startingLevel);
  freeResources(startingLevel, buffer, boundaries);
  return SUCCESS;
}

static void getEnvironmentVariables() {
  censoredMimes = malloc(sizeof(char *));
  censoredMimes[0] = NULL;
  char * msg = getenv("FILTER_MSG");
  if(msg != NULL)
    replacementBody = msg;
  char * mimes = getenv("FILTER_MEDIAS");
  splitMimes(mimes);
}

static void splitMimes(char * mimes) {
  if(mimes == NULL)
    return;
  int index, charCount = 0;
  for(index = 0; mimes[index] != '\0'; index++) {
    int count = relevantMimes - COMPOSITE_MIMES;
    if(charCount == 0) {
      censoredMimes[count] = realloc(censoredMimes[count], charCount + MIME_BLOCK);
    }
    if(mimes[index] == ',') {
      censoredMimes[count][charCount++] = '\0';
      charCount = 0;
      censoredMimes = realloc(censoredMimes, (count + 1) * sizeof(char *));
      relevantMimes++;
      censoredMimes[relevantMimes - COMPOSITE_MIMES] = NULL;
    }
    else if(!isspace(mimes[index])) {
      censoredMimes[count][charCount++] = mimes[index];
    }
  }
  int count = relevantMimes - COMPOSITE_MIMES;
  if(charCount % MIME_BLOCK == 0)
    censoredMimes[count] = realloc(censoredMimes[count], index + MIME_BLOCK);
  censoredMimes[count][charCount] = '\0';
  relevantMimes++;
}

static void freeResources(Level startingLevel, Buffer buffer, Stack boundaries/*, char ** Mimes*/) {
  free(buffer->buffer);
  free(buffer);
  emptyBoundaries(boundaries);
  free(boundaries);
  freeLevelResources(startingLevel);
  free(startingLevel);
  freeMimes();
}

static void freeMimes() {
  int index;
  for(index = 0; index < relevantMimes - COMPOSITE_MIMES; index++) {
    free(censoredMimes[index]);
  }
}

Buffer initializeBuffer() {
  Buffer buffer = malloc(sizeof(BufferCDT));
  buffer->buffer = malloc(BUF_BLOCK);
  buffer->i = 0;
  buffer->bytesRead = 0;
  buffer->doneReading = FALSE;
  return buffer;
}

ssize_t writeAndCheck(int fd, const void *buf, size_t count) {
  ssize_t bytesWritten = write(fd, buf, count);
  if(bytesWritten < 0)
    endWithFailure(errorMessages[WRITE_FAILURE]);
  return bytesWritten;
}

static void emptyBoundaries(Stack stack) {
  Boundary boundary = NULL;
  while((boundary = pop(stack)) != NULL) {
    free(boundary->bnd);
    free(boundary);
  }
}

Level initializeLevel() {
  Level level = malloc(sizeof(LevelCDT));
  resetLevel(level);
  return level;
}

static void resetLevel(Level level) {
  level->task = PARSING_POP3;
  level->taskStatus = NEW_LINE_POP3;
  level->headerParser = initializeHeaderParser(relevantMimes - COMPOSITE_MIMES);
  level->bodyParser = initializeBodyParser();
  
  level->isMultipart = FALSE;
  level->mustBeCensored = FALSE;
}

Boundary newBoundary(char * boundary, int length) {
  Boundary bound = malloc(sizeof(BoundaryCDT));
  bound->bnd = boundary;
  bound->length = length;
  return bound;
}

static void feedMoreBytes(Buffer buffer) {
  buffer->bytesRead = read(STDIN, buffer->buffer, BUF_BLOCK);
  buffer->i = 0;

  if(buffer->bytesRead == 0) {
    buffer->doneReading = TRUE;
  }
  else if(buffer->bytesRead < 0)
    endWithFailure(errorMessages[READ_FAILURE]);
}

static void consumeRemainingBytes(Level current) {
  if(!current->mustBeCensored) {
    int startingPoint = current->bodyParser->bufferStartingPoint;
    writeAndCheck(STDOUT, buffer->buffer + startingPoint, buffer->i - startingPoint + 1);
    while(!buffer->doneReading && buffer->bytesRead > 0) {
      feedMoreBytes(buffer);
      if(!buffer->doneReading)
        writeAndCheck(STDOUT, buffer->buffer, buffer->bytesRead);
    }
  }
  else {
    while(!buffer->doneReading && buffer->bytesRead > 0) {
      feedMoreBytes(buffer);
    }
    writeAndCheck(STDOUT, "\r\n.\r\n", 5);
  }
  return;
}

static void startConsuming(Level current) {
  while(!buffer->doneReading) {
    if(buffer->i == buffer->bytesRead) {
      if(current->task == PARSING_POP3) {
        writeAndCheck(STDOUT, buffer->buffer + pop3StartingPoint, buffer->i - pop3StartingPoint);
        pop3StartingPoint = 0;
      }
      if(!current->mustBeCensored && (current->task == PARSING_BODY 
        || current->task == DONE_PARSING_BODY || current->task == DONE_FINAL_BODY)) {
        int startingPoint = current->bodyParser->bufferStartingPoint;
        writeAndCheck(STDOUT, buffer->buffer + startingPoint, buffer->i - startingPoint);
      }
      current->bodyParser->bufferStartingPoint = 0;
      if(current->task != DONE_FINAL_BODY)
        feedMoreBytes(buffer); // reads more bytes, overwriting old buffer and setting i = 0
      if(current->task == PARSING_HEADERS 
        && buffer->doneReading && buffer->i == buffer->bytesRead) {
        write(STDOUT, current->headerParser->hbuf, current->headerParser->hbufIndex + 1);
      }
    }

    for( ; buffer->i < buffer->bytesRead; buffer->i++) {
      switch(current->task) {
        case PARSING_POP3:                parsePop3(current, buffer->buffer[buffer->i]);
                                          break;
        case PARSING_HEADERS:             parseHeader(current, buffer->buffer[buffer->i]); // pushea boundary si es multipart
                                          break;
        case DONE_PARSING_HEADERS:        if(current->isMultipart && !current->mustBeCensored) {
                                            Boundary nextBoundary = current->headerParser->nextBoundary;
                                            push(boundaries, nextBoundary);
                                          }
                                          else {
                                            free(current->headerParser->nextBoundary->bnd);
                                            free(current->headerParser->nextBoundary);
                                          }
                                          current->task = PARSING_BODY;
                                          current->taskStatus = NEW_LINE_BODY;
                                          current->bodyParser->bufferStartingPoint = buffer->i;
                                          resetHeaderParser(current->headerParser, relevantMimes - COMPOSITE_MIMES); // free all headers
                                          parseBody(current, buffer->buffer[buffer->i], peek(boundaries));
                                          break;
        case PARSING_BODY:                parseBody(current, buffer->buffer[buffer->i], peek(boundaries)); // pops boundary when needed
                                          break;
        case DONE_PARSING_BODY:           if(!current->mustBeCensored) { // writes remaining body bytes
                                            int startingPoint = current->bodyParser->bufferStartingPoint;
                                            writeAndCheck(STDOUT, buffer->buffer + startingPoint, 
                                              buffer->i - startingPoint + 1);
                                          }
                                          resetLevelValues(current); // sets PARSING_HEADERS, NEW_LINE
                                          resetBodyParser(current->bodyParser);
                                          break;
        case DONE_FINAL_BODY:             return;
      }
    }
  }
  
  return;
}

static void parsePop3(Level current, char c) {
  switch(current->taskStatus) {
    case NEW_LINE_POP3:           if(c == '+' || c == '-') {
                                    current->taskStatus = COMMAND_POP3;
                                  }
                                  else {
                                    current->task = PARSING_HEADERS;
                                    current->taskStatus = NEW_LINE;
                                    writeAndCheck(STDOUT, buffer->buffer + pop3StartingPoint,
                                      buffer->i - pop3StartingPoint);
                                    parseHeader(current, c);
                                    return;
                                  }
                                  break;
    case CR_POP3:                 if(c == '\n') {
                                    current->taskStatus = NEW_LINE_POP3;
                                  }
                                  break;
    case COMMAND_POP3:            if(c == '\r') {
                                    current->taskStatus = CR_POP3;
                                  }
                                  break;
  }
}

static void writeBoundary(bool isFinalPart, Boundary currentBoundary) {
  if(currentBoundary == NULL)
    return;
  int size = HYPHENS + currentBoundary->length + CRLF;
  char * bnd = NULL;
  if(isFinalPart) {
    size += HYPHENS;
    bnd = malloc(size);
    bnd[size-CRLF-HYPHENS] = bnd[size-CRLF-HYPHENS+1] = '-';
  }
  else
    bnd = malloc(size);
  bnd[0] = bnd[1] = '-';
  bnd[size-CRLF] = '\r';
  bnd[size-CRLF+1] = '\n';
  memcpy(bnd + HYPHENS, currentBoundary->bnd, 
    size - CRLF - HYPHENS - isFinalPart * HYPHENS);
  writeAndCheck(STDOUT, bnd, size);
  free(bnd);
}

static void freeLevelResources(Level level) {
  freeHeaderParser(level->headerParser);
  free(level->bodyParser);
}

static void parseHeader(Level current, char c) {
  HeaderParser hp = current->headerParser;
  switch(current->taskStatus) {
    case NEW_LINE:              ;
                                bool shouldPrintLastHeader = hp->hbufIndex > 0 && matchesFound(hp) == -1; // si tengo mas de un char y no es header relevante
                                if(c == '\r') {
                                  break;
                                }
                                if(c == '\n') {
                                  if(shouldPrintLastHeader) {
                                    writeAndCheck(STDOUT, hp->hbuf, hp->hbufIndex); // si estoy en el ultimo header pre body
                                    hp->hbufIndex = 0;
                                  }
                                  if(!current->mustBeCensored) {
                                    putHeaders(hp->relevantHeaders);
                                    if(hp->isRfc822Mime) {
                                      writeAndCheck(STDOUT, "\r\n", 2);
                                      resetHeaderParser(hp, relevantMimes - COMPOSITE_MIMES);
                                      current->task = PARSING_HEADERS;
                                      break;
                                    }
                                  }
                                  if(current->mustBeCensored) {
                                    injectReplacementHeaderAndBody();
                                  }
                                  else {
                                    writeAndCheck(STDOUT, "\r\n", 2);
                                  }
                                  current->task = DONE_PARSING_HEADERS; // DONE_PARSING_HEADERS
                                  break;
                                }
                                if(isspace(c)) {
                                  current->taskStatus = hp->lastTaskStatus;
                                  if(hp->lastTaskStatus == HEADER_BODY 
                                    || hp->lastTaskStatus == CONTENT_TYPE_BODY) {
                                    addCharToHeaderBody(hp, matchesFound(hp),c);
                                    if(hp->lastTaskStatus == CONTENT_TYPE_BODY) {
                                      hp->ctp->wspAndNewLines++;
                                    }
                                    break;
                                  }
                                  addCharToHeaderBuffer(hp, c);
                                  break;
                                }
                                if(shouldPrintLastHeader) {
                                  writeAndCheck(STDOUT, hp->hbuf, hp->hbufIndex);
                                  hp->hbufIndex = 0;
                                }
                                hp->hbufIndex = 0;
                                setToTrueIfNotFound(hp->potentialRelevantHeader,
                                  RELEVANT_HEADERS);
                                bool matching = checkIsRelevantHeader(current, c);
                                if(!matching)
                                  current->taskStatus = IRRELEVANT_HEADER;
                                else
                                  current->taskStatus = RELEVANT_HEADER_NAME;
                                addCharToHeaderBuffer(hp, c);
                                break;
    case CR_HEADER:             if(c == '\n') {
                                  current->taskStatus = NEW_LINE;
                                  if(hp->lastTaskStatus == HEADER_BODY 
                                    || hp->lastTaskStatus == CONTENT_TYPE_BODY) {
                                    addCharToHeaderBody(hp, matchesFound(hp), c);
                                  }
                                  else {
                                    addCharToHeaderBuffer(hp, c);
                                  }
                                  break;
                                }
                                current->taskStatus = HEADER_BODY;
                                break;
    case RELEVANT_HEADER_NAME:  if(c == '\r') {
                                  hp->lastTaskStatus = RELEVANT_HEADER_NAME;
                                  current->taskStatus = CR_HEADER;
                                  break;
                                }
                                int matchingIndex = matchesFound(hp);
                                if((c == ':' || isspace(c))) {
                                  if(!anyFinishedMatch(hp)) {
                                    current->taskStatus = IRRELEVANT_HEADER;
                                    hp->potentialRelevantHeader[matchingIndex] = FALSE;
                                  }
                                  else {
                                    if(matchingIndex == 0) {
                                      current->taskStatus = CONTENT_TYPE_BODY;
                                      hp->ctp->contentTypeStatus = MIME;
                                    }
                                    else {
                                      current->taskStatus = HEADER_BODY;
                                    }
                                    hp->relevantHeaders[matchingIndex] = initializeHeader();
                                    memcpy(hp->relevantHeaders[matchingIndex]->name,hp->hbuf, hp->hbufIndex);
                                    hp->relevantHeaders[matchingIndex]->nameLength = hp->hbufIndex + 1;
                                    hp->relevantHeaders[matchingIndex]->name[hp->hbufIndex] = ':';
                                  }
                                  break;
                                }
                                bool match = checkIsRelevantHeader(current, c);
                                if(!match)
                                  current->taskStatus = IRRELEVANT_HEADER;
                                addCharToHeaderBuffer(hp, c);
                                break;
    case IRRELEVANT_HEADER:     addCharToHeaderBuffer(hp, c);
                                hp->lastTaskStatus = IRRELEVANT_HEADER;
                                if(c == '\r') {
                                  current->taskStatus = CR_HEADER;
                                  break;
                                }
                                break;
    case HEADER_BODY:           hp->lastTaskStatus = HEADER_BODY;
                                int mtch = matchesFound(hp);
                                addCharToHeaderBody(hp, mtch, c);
                                if(c == '\r') {
                                  current->taskStatus = CR_HEADER;
                                  break;
                                }
                                break;
    case CONTENT_TYPE_BODY:     parseHeaderParams(current, c);
                                hp->lastTaskStatus = CONTENT_TYPE_BODY;
                                break;
  }
}

static bool anyFinishedMatch(HeaderParser hp) {
  int index;
  bool ret = FALSE;
  bool aux = FALSE;
  for(index = 0; index < RELEVANT_HEADERS; index++) {
    if(hp->potentialRelevantHeader[index] == TRUE) {
      aux = cmp(relevantHeaderNames[index], hp->hbufIndex, '\0');
    }
    if(aux)
      ret = TRUE;
  }
  return ret;
}

static void parseHeaderParams(Level current, char c) {
  HeaderParser hp = current->headerParser;
  switch(hp->ctp->contentTypeStatus) {
    case CR_MIME:                 if(c == '\n') {
                                    hp->ctp->contentTypeStatus = NEW_LINE_CONTENT_TYPE;
                                  }
                                  break;
    case MIME:                    ;
                                  int len = hp->relevantHeaders[CONTENT_TYPE]->bodyLength - hp->ctp->wspAndNewLines;
                                  if((c == '\r' || c == ';')) {
                                    hp->ctp->wspAndNewLines += 2;
                                    int nextStatus = CONTENT_TYPE_BODY;
                                    int currentStatus = CONTENT_TYPE_BODY;// dont need to parse anything else about this header
                                    if(anyCensoredMatches(hp->ctp->potentialRelevantMime, len)) {
                                      current->mustBeCensored = TRUE;
                                      nextStatus = HEADER_BODY;
                                      currentStatus = HEADER_BODY;
                                    }
                                    else if(isComposite(hp->ctp->potentialMultipart, len, MULTIPART)) {
                                      current->isMultipart = TRUE;
                                      hp->ctp->contentTypeStatus = SEPARATOR;
                                    }
                                    else if(isComposite(hp->ctp->potentialRfc822, len, RFC822)) {
                                      hp->isRfc822Mime = TRUE;
                                    }
                                    if(c == '\r')
                                      nextStatus = CR_HEADER;
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                    current->taskStatus = nextStatus;
                                    hp->lastTaskStatus = currentStatus;
        
                                    break;
                                  }
                                  if(isspace(c) || c == ':') {
                                    break;
                                  }
                                  if(c == '(') {
                                    hp->ctp->previousContentTypeStatus = MIME;
                                    hp->ctp->contentTypeStatus = COMMENT;
                                    break;
                                  }
                                  if(c == ';' && hp->ctp->potentialMultipart 
                                    && hp->relevantHeaders[CONTENT_TYPE]->bodyLength >= strlen(compositeMimes[MULTIPART])) {
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                    current->isMultipart = TRUE;
                                    hp->ctp->contentTypeStatus = SEPARATOR;
                                    break;
                                  }
                                  int matching = checkIsRelevantMime(hp->ctp->potentialRelevantMime, 
                                    len, c);
                                  hp->ctp->potentialMultipart = checkCompositeMime(len, c, MULTIPART, 
                                    hp->ctp->potentialMultipart);
                                  hp->ctp->potentialRfc822 = checkCompositeMime(len, c, RFC822,
                                    hp->ctp->potentialRfc822);
                                  addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                  if(!matching && !hp->ctp->potentialMultipart
                                    && !hp->ctp->potentialRfc822) {
                                    current->taskStatus = HEADER_BODY;
                                    return;
                                  }
                                  break;
    case SEPARATOR:               if(c == '\r') {
                                    addCharToHeaderBody(hp, CONTENT_TYPE, '\r');
                                    hp->ctp->previousContentTypeStatus = SEPARATOR;
                                    current->taskStatus = CR_HEADER;
                                    break;
                                  }
                                  if(c == '(') {
                                    hp->ctp->previousContentTypeStatus = SEPARATOR;
                                    hp->ctp->contentTypeStatus = COMMENT;
                                    break;
                                  }
                                  if(isspace(c))
                                    break;
                                  if(c == '=' && hp->ctp->boundaryNameIndex == strlen(boundaryName)) {
                                    hp->ctp->contentTypeStatus = BOUNDARY;
                                    addCharToHeaderBody(hp, CONTENT_TYPE, '=');
                                    break;
                                  }
                                  addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                  if(tolower(c) != boundaryName[hp->ctp->boundaryNameIndex++]) {
                                    hp->ctp->boundaryNameIndex = 0;
                                    break;
                                  }
                                  break;
    case COMMENT:                 if(c == ')') {
                                    hp->ctp->contentTypeStatus = hp->ctp->previousContentTypeStatus;
                                    break;
                                  }
                                  break;
    case BOUNDARY:                if(c == '\r' && (hp->ctp->doubleQuoteCount == 0 
                                    || hp->ctp->doubleQuoteCount == 2)) {
                                    addCharToHeaderBody(hp, CONTENT_TYPE, '\r');
                                    current->taskStatus = CR_HEADER;
                                    hp->lastTaskStatus = HEADER_BODY;
                                    break;
                                  }
                                  if(c == '(') {
                                    hp->ctp->previousContentTypeStatus = SEPARATOR;
                                    hp->ctp->contentTypeStatus = COMMENT;
                                    break;
                                  }
                                  else if(c == '\r' || c == '\n') { // boundary="simpl
                                    break;                          // eBoundary" is a valid boundary, 
                                  }                                 // note it has no folding WSP
                                  if(c == '\"') {
                                    hp->ctp->doubleQuoteCount++;
                                    if(hp->ctp->doubleQuoteCount == 2 && !current->mustBeCensored) {
                                    }
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                    break;
                                  }
                                  if(hp->ctp->doubleQuoteCount != 2) {
                                    addCharToBoundary(hp, c);
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                  }
                                  break;
  }
}

static void addCharToHeaderBuffer(HeaderParser hp, char c) {
  if(hp->hbufIndex % HBUF_BLOCK == 0)
    hp->hbuf = realloc(hp->hbuf, hp->hbufIndex + HBUF_BLOCK);
  hp->hbuf[hp->hbufIndex++] = c;
}

static void addCharToHeaderBody(HeaderParser hp, int position, char c) {
  Header h = hp->relevantHeaders[position];
  if(position % HBUF_BLOCK == 0)
    h->body = realloc(h->body, h->bodyLength + HBUF_BLOCK);
  h->body[h->bodyLength++] = c;
}

static void addCharToBoundary(HeaderParser hp, char c) {
  Boundary b = hp->nextBoundary;
  if(b->length % HBUF_BLOCK == 0)
    b->bnd = realloc(b->bnd, b->length + HBUF_BLOCK);
  b->bnd[b->length++] = c;
}

static bool checkIsRelevantHeader(Level current, char c) {
  int index;
  bool ret = FALSE;
  HeaderParser hp = current->headerParser;
  for(index = 0; index < RELEVANT_HEADERS; index++) {
    if(hp->potentialRelevantHeader[index] == TRUE) {
      hp->potentialRelevantHeader[index] = cmp(relevantHeaderNames[index], hp->hbufIndex, c);
    }
    if(hp->potentialRelevantHeader[index] == TRUE)
      ret = TRUE;
  }
  return ret;
}

static bool checkIsRelevantMime(char * potentialRelevantMime, int position, char c) {
  int index;
  bool ret = FALSE;
  for(index = 0; index < relevantMimes - COMPOSITE_MIMES; index++) {
    if(potentialRelevantMime[index] == TRUE) {
      potentialRelevantMime[index] = cmpMime(censoredMimes[index], position, c);
    }
    if(potentialRelevantMime[index] == TRUE)
      ret = TRUE;
  }
  return ret;
}

static bool checkCompositeMime(int position, char c, int compositeMime, char potential) {
  if(potential == TRUE) {
    return cmpMime(compositeMimes[compositeMime], position, c);//tolower(c) == compositeMimes[compositeMime][position];
  }
  return FALSE;
}

static bool cmpMime(const char * str, int position, char c) {
  int len = strlen(str);
  if(position >= len - 1 && str[len - 1] == '*') {
    return TRUE;
  }
  if(position > len) {
    return FALSE;
  }
  return tolower(c) == tolower(str[position]);
}

static bool anyCensoredMatches(const char * potentialRelevantMime, int position) {
  int index;
  for(index = 0; index < relevantMimes - COMPOSITE_MIMES; index++) {
    if(potentialRelevantMime[index] == TRUE && cmpMime(censoredMimes[index], position, '\0'))
      return TRUE;
  }
  return FALSE;
}

static bool isComposite(char potentiallyComposite, int position, int mime) {
  return potentiallyComposite && cmpMime(compositeMimes[mime], position, '\0');
}

static bool cmp(const char * str, int position, char c) {
  if(position > strlen(str)) // always compare to null-terminated str
    return FALSE;
  return tolower(c) == tolower(str[position]);
}

static void setToTrueIfNotFound(char * arr, int size) {
  int index;
  for(index = 0; index < size; index++) {
    if(arr[index] == 0)
      arr[index] = TRUE;
  }
}

static void injectReplacementHeaderAndBody() {
  writeAndCheck(STDOUT, replacementHeader->name, replacementHeader->nameLength);
  writeAndCheck(STDOUT, replacementHeader->body, replacementHeader->bodyLength);
  writeAndCheck(STDOUT, replacementBody, strlen(replacementBody));
  writeAndCheck(STDOUT, "\r\n", 2);
}

/* RFC:
 * NOTE: The CRLF preceding the encapsulation line is considered part of the boundary 
 * so that it is possible to have a part that does not end with a CRLF (line break). 
 * Body parts that must be considered to end with line breaks, therefore, should have
 * two CRLFs preceding the encapsulation line, the first of which is part of the 
 * preceding body part, and the second of which is part of the encapsulation boundary.
 */
static void parseBody(Level current, char c, Boundary boundary) {
  if(boundary == NULL) {
    if(buffer->i == buffer->bytesRead - 1
      || (current->mustBeCensored && buffer->i == buffer->bytesRead - 1)) {
      current->task = DONE_FINAL_BODY;
      current->taskStatus = WRITING_NO_CHECK;
      consumeRemainingBytes(current);
      return;
    }
    return;
  }
  switch(current->taskStatus) {
    case NEW_LINE_BODY:               if(c == '-') {
                                        current->bodyParser->hyphenCount++;
                                        current->taskStatus = HYPHEN;
                                        break;
                                      }
                                      if(c == '\r') {
                                        current->taskStatus = CR_BODY;
                                        break;
                                      }
                                      if(c == '\n')
                                        break;
                                      current->taskStatus = IRRELEVANT_BODY;
                                      break;
    case CR_BODY:                     if(c == '\n') {
                                        current->taskStatus = NEW_LINE_BODY;
                                        break;
                                      }
                                      current->taskStatus = IRRELEVANT_BODY;
                                      break;
    case HYPHEN:                      if(c == '-') {
                                        current->bodyParser->hyphenCount++;
                                      }
                                      if(current->bodyParser->hyphenCount == HYPHENS) {
                                        current->taskStatus = SEARCHING_BOUNDARY;
                                        break;
                                      }
                                      current->bodyParser->hyphenCount = 0;
                                      current->taskStatus = IRRELEVANT_BODY;
                                      break;
    case SEARCHING_BOUNDARY:          current->bodyParser->hyphenCount = 0;
                                      if(c == '\r') {
                                        current->taskStatus = CR_BODY;
                                        break;
                                      }
                                      if(c == boundary->bnd[current->bodyParser->boundaryIndex]) {
                                        current->bodyParser->boundaryIndex++;
                                        if(current->bodyParser->boundaryIndex == boundary->length) {
                                          current->taskStatus = BOUNDARY_POP_CHECK;
                                          current->bodyParser->boundaryIndex = 0;
                                        }
                                        break;
                                      }
                                      current->bodyParser->boundaryIndex = 0;
                                      current->taskStatus = IRRELEVANT_BODY;
                                      break;
    case IRRELEVANT_BODY:             if(c == '\r') {
                                        current->taskStatus = CR_BODY;
                                        break;
                                      }
                                      break;
    case BOUNDARY_POP_CHECK:          if(c == '\r') {
                                        current->task = DONE_PARSING_BODY;
                                      }
                                      if(c == '-') {
                                        current->bodyParser->hyphenCount++;
                                        if(current->bodyParser->hyphenCount < HYPHENS)
                                          break;
                                      }
                                      if(current->bodyParser->hyphenCount == HYPHENS) {
                                        current->bodyParser->isFinalPart = TRUE;
                                        Boundary popped = pop(boundaries);
                                        if(current->mustBeCensored)
                                          writeBoundary(current->bodyParser->isFinalPart, boundary);
                                        free(popped->bnd);
                                        free(popped);
                                        current->task = PARSING_BODY;
                                        current->taskStatus = NEW_LINE_BODY;
                                        resetBodyParserSameStartingPoint(current->bodyParser);
                                        break;
                                      }
                                      if(current->mustBeCensored) {
                                        writeBoundary(current->bodyParser->isFinalPart, peek(boundaries));
                                      }
                                      break;
  }
}

void resetLevelValues(Level level) {
  level->task = PARSING_HEADERS;
  level->taskStatus = NEW_LINE;
  level->isMultipart = FALSE;
  level->mustBeCensored = FALSE;
  resetHeaderParser(level->headerParser, relevantMimes - COMPOSITE_MIMES);
}

int caseInsensitiveStrncmp(const char * s1, const char * s2, size_t n) {
  if(s1 == NULL || s2 == NULL) {
    return -1;
  }
  while (n && *s1 && (tolower(*s1) == tolower(*s2))) {
    ++s1;
    ++s2;
    --n;
  }
  if(n == 0) {
    return 0;
  }
  else {
    return (tolower(*s1) - tolower(*s2));
  }
}

int endWithFailure(const char * msg) {
  if(errno == 0)
    errno = -1;
  if(msg != NULL)
    perror(msg);
  exit(EXIT_FAILURE);
}
