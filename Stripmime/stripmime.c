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
void resetLevelValues(Level);

char * hstat[] =  {"","NEW_LINE","CR_HEADER","RELEVANT_HEADER_NAME",
"IRRELEVANT_HEADER","HEADER_BODY","CONTENT_TYPE_BODY"};
/*
    KNOWN BUGS: |  Content-Type:image/png
                |     haha
    will be censored if image/png is censored
*/

/*
 * Posibles escenarios:
 * - Un solo MIME type (header Content-Type presente)
 * - Multipart/mixed (headers presentes)
 * - Multipart anidados (headers presentes)
 * - Message/rfc822 o cualquier subtipode message
 * - Algun text/plain presente con exactamente el formato que busco (headers y todo)
 * - Ningun header MIME --> Content-Type: text/plain; encoding=us-ascii
 * - Ningun header y con la palabra Content-Type
 * VER --> man wordexp
 * fuzzer / fuzzing
 * fork y exec con bash -c 
 */

    // "Content-Type:"
    /*
    HEADERS:
    - Formato: <FIELD_NAME> <COLON> <FIELD_BODY>
    Field body puede estar dividido en multiples lineas siempre y cuando despues
    del CRLF venga WSP (WSP == WhiteSPace)
    Apache cs->jMeter
    Address sanitizer: en tiempo de compilacion el comp va a ubicar las variables
    de cierta forma y salta cuando se encuentre algun acceso invalido a memoria
    hace falta poner un -f
    analisis estatico de codigo (cppcheck)
    COMENTARIO EN HEADER CONTENT-TYPE PUEDE TENER COMENTARIO ANTES DE QUE APAREZCA
    EL MIME
    */

Stack boundaries = NULL; // stack containing non-censored multipart boundaries
Buffer buffer = NULL; // universal buffer
char * replacementBody = "This is the default replacement message. \
 Your message has been censored.";
int relevantMimes = COMPOSITE_MIMES;
char ** censoredMimes = NULL;

int main(void) {
  putenv("FILTER_MSG=Te censure");
  putenv("FILTER_MEDIAS=image/jpeg,image/png");
  getEnvironmentVariables();
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
     //write(STDOUT, censoredMimes[count], strlen(censoredMimes[count]));
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
  level->task = PARSING_HEADERS; // PARSING_HEADERS, DONE_PARSING_HEADERS, PARSING_BODY, DONE_PARSING_BODY
  level->taskStatus = NEW_LINE; // NEW_LINE, HEADER_NAME, HEADER_BODY, SEARCHING_BOUNDARY
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
   //printf("Tengo que imprimir lo que queda\n");
    int startingPoint = current->bodyParser->bufferStartingPoint;
    //write(STDOUT, "Tengo que escribir lo que queda", strlen("Tengo que escribir lo que queda"));
    writeAndCheck(STDOUT, buffer->buffer + startingPoint, buffer->i - startingPoint + 1);
    while(!buffer->doneReading && buffer->bytesRead > 0) {
      feedMoreBytes(buffer);
      //write(STDOUT, "ESCRIBIENDO TODO!", strlen("escribiendo todo!"));
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
      if(!current->mustBeCensored && (current->task == PARSING_BODY 
        || current->task == DONE_PARSING_BODY || current->task == DONE_FINAL_BODY)) {
        int startingPoint = current->bodyParser->bufferStartingPoint;
        writeAndCheck(STDOUT, buffer->buffer + startingPoint, buffer->i - startingPoint);
        //write(STDOUT, " --IMPRIMI--\n", strlen(" --IMPRIMI--\n"));
      }
      current->bodyParser->bufferStartingPoint = 0;
      if(current->task != DONE_FINAL_BODY)
        feedMoreBytes(buffer); // reads more bytes, overwriting old buffer and setting i = 0
      if(current->task == PARSING_HEADERS && buffer->doneReading) {
        //manageLastHeadersWithEmptyBody();
      }
    }
//char aux[400];
//sprintf(aux, "Status: %d; i: %d; bytesRead: %d; doneReading: %d", current->task, buffer->i, buffer->bytesRead, buffer->doneReading);
//write(STDOUT, aux, strlen(aux));
    for( ; buffer->i < buffer->bytesRead; buffer->i++) {
      switch(current->task) {
        case PARSING_HEADERS:             parseHeader(current, buffer->buffer[buffer->i]); // pushea boundary si es multipart
                                          break;
        case DONE_PARSING_HEADERS:        if(current->isMultipart && !current->mustBeCensored) {
                                            Boundary nextBoundary = current->headerParser->nextBoundary;
                                            push(boundaries, nextBoundary);
  //write(STDOUT, "PRINT BOUNDARIES ", strlen("PRINT BOUNDARIES "));
  //write(STDOUT, ((Boundary)peek(boundaries))->bnd, ((Boundary)peek(boundaries))->length);
  //write(STDOUT, "\r\n", 2);
                                          }
                                          else {
                                            free(current->headerParser->nextBoundary->bnd);
                                            free(current->headerParser->nextBoundary);
                                          }
                                          current->task = PARSING_BODY;
                                          current->taskStatus = NEW_LINE_BODY;
                                          current->bodyParser->bufferStartingPoint = buffer->i;
                                          resetHeaderParser(current->headerParser, relevantMimes - COMPOSITE_MIMES); // free all headers
                                          //printf("Index: %d, char: %c\n", buffer->i, buffer->buffer[buffer->i]);
                                          parseBody(current, buffer->buffer[buffer->i], peek(boundaries));
                                          break;
        case PARSING_BODY:                parseBody(current, buffer->buffer[buffer->i], peek(boundaries)); // popeara boundary cuando lo necesite
                                          break;
        case DONE_PARSING_BODY:           if(!current->mustBeCensored) { // writes remaining body bytes
                                            int startingPoint = current->bodyParser->bufferStartingPoint;
                                            writeAndCheck(STDOUT, buffer->buffer + startingPoint, 
                                              buffer->i - startingPoint + 1);
                                          }
                                          resetLevelValues(current); // sets PARSING_HEADERS, NEW_LINE
                                          resetBodyParser(current->bodyParser);
                                          /*if(buffer->buffer[buffer->i] == '\n')
                                            putchar('?');
                                          else if(buffer->buffer[buffer->i] == '\r')
                                            putchar('!');
                                          else
                                            putchar(buffer->buffer[buffer->i]);*/
                                          // if(peek(boundaries) == NULL) {
                                          //   //printf("Se acabo todo\n");
                                          //   consumeRemainingBytes(current);
                                          //   return;
                                          // }
                                          //printf("Starting to parse headers\n");
                                          break;
        case DONE_FINAL_BODY:             return;
      }
    }
  }
  
  return;
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
  //printf("Status: (%d) %s\n", current->taskStatus, hstat[current->taskStatus]);
  switch(current->taskStatus) {
    case NEW_LINE:              ;//printf("Status: %s __Char:%c\n", hstat[current->taskStatus], c);
                                //printf("Antes de write, hbufindex: %d, matchesFound: %d\n", hp->hbufIndex, matchesFound(hp));
                                bool shouldPrintLastHeader = hp->hbufIndex > 0 && matchesFound(hp) == -1; // si tengo mas de un char y no es header relevante
                                if(c == '\r') {
                                  break;
                                }
                                if(c == '\n') {
                                  if(shouldPrintLastHeader) {
                                    //printf("Printeo, matchesFound: %d\n", matchesFound(hp));
                                    writeAndCheck(STDOUT, hp->hbuf, hp->hbufIndex); // si estoy en el ultimo header pre body
                                    hp->hbufIndex = 0;
                                  }
                                  if(!current->mustBeCensored) {
                                    //printf("No debo censurar y puse todo\n");
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
                                  // printf("Encontre espacio\n");
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
                                //printf("Matching: %c, %d\n",c,  matching);
                                if(!matching)
                                  current->taskStatus = IRRELEVANT_HEADER;
                                else
                                  current->taskStatus = RELEVANT_HEADER_NAME;
                                addCharToHeaderBuffer(hp, c);
                                break;
    case CR_HEADER:             //printf("Status: %s __Char:%c\n", hstat[current->taskStatus], c);
                                if(c == '\n') {
                                  current->taskStatus = NEW_LINE;
                                  if(hp->lastTaskStatus == HEADER_BODY 
                                    || hp->lastTaskStatus == CONTENT_TYPE_BODY) {
                                    addCharToHeaderBody(hp, matchesFound(hp), c);
                                  }
                                  else {
                                    addCharToHeaderBuffer(hp, c);
                                  }
                                  /*if(hp->lastTaskStatus == CONTENT_TYPE_BODY)
                                    hp->ctp->wspAndNewLines++;*/
                                  break;
                                }
                                current->taskStatus = HEADER_BODY;
                                break;
    case RELEVANT_HEADER_NAME:  ;
// char aux[400] = {0};
//     sprintf(aux, "Status: %s __Char:%c\n", hstat[current->taskStatus], c);
//     write(STDOUT, aux, strlen(aux));
                                if(c == '\r') {
                                  hp->lastTaskStatus = RELEVANT_HEADER_NAME;
                                  current->taskStatus = CR_HEADER;
                                  break;
                                }
                                int matchingIndex = matchesFound(hp);
                                if((c == ':' || isspace(c))) {
                                  //printf("Termino %c\n", (relevantHeaderNames[matchingIndex][hp->hbufIndex - 1] != 0) + '0');
                                  if(!anyFinishedMatch(hp)) {
                                    current->taskStatus = IRRELEVANT_HEADER;
                                    hp->potentialRelevantHeader[matchingIndex] = FALSE;
                                  }
                                  else {
                                    if(matchingIndex == 0) {
                                      //printf("Matching index 0 y CONTENT_TYPE\n");
                                      current->taskStatus = CONTENT_TYPE_BODY;
                                      hp->ctp->contentTypeStatus = MIME;
                                    }
                                    else {
                                      //printf("Matching index 1 y HEADER_BODY\n");
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
    case IRRELEVANT_HEADER:     //printf("Status: %s __Char:%c\n", hstat[current->taskStatus], c);
                                addCharToHeaderBuffer(hp, c);
                                hp->lastTaskStatus = IRRELEVANT_HEADER;
                                if(c == '\r') {
                                  current->taskStatus = CR_HEADER;
                                  break;
                                }
                                break;
    case HEADER_BODY:           //addCharToHeaderBody(hp, c); // busco el unico que tiene 1 en el char * potential
                                //printf("Status: %s __Char:%c\n", hstat[current->taskStatus], c);
                                hp->lastTaskStatus = HEADER_BODY;
                                int mtch = matchesFound(hp);
                                addCharToHeaderBody(hp, mtch, c);
                                if(c == '\r') {
                                  //addCharToHeaderBody(hp, mtch, '\r');
                                  //addCharToHeaderBody(hp, mtch, '\n');
                                  current->taskStatus = CR_HEADER;
                                  break;
                                }
                                break;
    case CONTENT_TYPE_BODY:     //printf("Status: %s __Char:%c\n", hstat[current->taskStatus], c);
                                parseHeaderParams(current, c);
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
// char aux[400] = {0};
// sprintf(aux,"Status: %s __Char:%c\n", cs[hp->ctp->contentTypeStatus], c);
// write(STDOUT, aux, strlen(aux));
  switch(hp->ctp->contentTypeStatus) {
    case CR_MIME:                 if(c == '\n') {
                                    hp->ctp->contentTypeStatus = NEW_LINE_CONTENT_TYPE;
                                  }
                                  break;
    case MIME:                    ;//printf("Status: %s __Char:%c\n", cs[hp->ctp->contentTypeStatus], (c == '\r') ? '+' : c);
    // write(STDOUT, "antes len ", strlen("antes len "));
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
                                      //printf("ES MULTIPART\n");
                                      current->isMultipart = TRUE;
                                      hp->ctp->contentTypeStatus = SEPARATOR;
                                    }
                                    else if(isComposite(hp->ctp->potentialRfc822, len, RFC822)) {
                                      //printf("Me dio true, es MESSAGE/RFC822\n");
                                      hp->isRfc822Mime = TRUE;
                                    }
                                    /*if(c == '\r') {
                                      addCharToHeaderBody(hp, CONTENT_TYPE, '\r');
                                      //addCharToHeaderBody(hp, CONTENT_TYPE, '\n');
                                      nextStatus = CR_HEADER;
                                    }*/
                                    if(c == '\r')
                                      nextStatus = CR_HEADER;
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                              //char aux[4000] = {0};
                              //sprintf(aux, "me estoy yendo, isMultipart: %d, ismessage: %d, censurable: %d\n", current->isMultipart, hp->isRfc822Mime, current->mustBeCensored);
                              //write(STDOUT, aux, strlen(aux));
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
                    // write(STDOUT, "antes matching ", strlen("antes matching "));
                    // char auxi[400] = {0};
                    // sprintf(auxi, "ctp: %p , potentialRelevantMime: %p ", hp->ctp, hp->ctp->potentialRelevantMime);
                    // write(STDOUT, auxi, strlen(auxi));
                                  int matching = checkIsRelevantMime(hp->ctp->potentialRelevantMime, 
                                    len, c);
                    // write(STDOUT, "dsps matching ", strlen("dsps matching "));
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
    case SEPARATOR:               //printf("Status: %s __Char:%c\n", cs[hp->ctp->contentTypeStatus], c);
                                  if(c == '\r') {
                                    addCharToHeaderBody(hp, CONTENT_TYPE, '\r');
                                    hp->ctp->previousContentTypeStatus = SEPARATOR;
                                    current->taskStatus = CR_HEADER;
                                    break;
                                  }
                                  if(c == '(') {
                                    hp->ctp->previousContentTypeStatus = SEPARATOR;
                                    hp->ctp->contentTypeStatus = COMMENT;
                                    //hp->ctp->wspAndNewLines++;
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
    case COMMENT:                 //printf("Status: %s __Char:%c\n", cs[hp->ctp->contentTypeStatus], c);
                                  if(c == ')') {
                                    //hp->ctp->wspAndNewLines++;
                                    hp->ctp->contentTypeStatus = hp->ctp->previousContentTypeStatus;
                                    break;
                                  }
                                  //hp->ctp->wspAndNewLines++;
                                  break;
    case BOUNDARY:                //printf("Status: %s __Char:%c\n", cs[hp->ctp->contentTypeStatus], c);
                                  if(c == '\r' && (hp->ctp->doubleQuoteCount == 0 
                                    || hp->ctp->doubleQuoteCount == 2)) {
                                    addCharToHeaderBody(hp, CONTENT_TYPE, '\r');
                     //           write(STDOUT, "BOUNDARY: ", strlen("BOUNDARY: "));
                    //write(STDOUT, hp->nextBoundary->bnd, hp->nextBoundary->length);
                      //  write(STDOUT, "FIN\r\n", 5);
                                    current->taskStatus = CR_HEADER;
                                    hp->lastTaskStatus = HEADER_BODY;
                                    break;
                                  }
                                  if(c == '(') {
                                    hp->ctp->previousContentTypeStatus = SEPARATOR;
                                    hp->ctp->contentTypeStatus = COMMENT;
                                    //hp->ctp->wspAndNewLines++;
                                    break;
                                  }
                                  else if(c == '\r' || c == '\n') { // boundary="simpl
                                    break;                          // eBoundary" is a valid boundary, 
                                  }                                 // note it has no folding WSP
                                  if(c == '\"') {
                                    hp->ctp->doubleQuoteCount++;
                                    if(hp->ctp->doubleQuoteCount == 2 && !current->mustBeCensored) {
                                      //finishBoundary();
                                    }
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                                    break;
                                  }
                                  if(hp->ctp->doubleQuoteCount != 2) {
                          //write(STDOUT, "Antes de addCharBoundary", strlen("Antes de addCharBoundary"));
                                    addCharToBoundary(hp, c);
                          //write(STDOUT, "Antes de addCharHeader", strlen("Antes de addCharHeader"));
                                    addCharToHeaderBody(hp, CONTENT_TYPE, c);
                          //write(STDOUT, "Escribi doble", strlen("Escribi doble"));
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
//char aux[400] = {0};
//sprintf(aux, "BodyLength: %d; char: %c", h->bodyLength, c);
//write(STDOUT, aux, strlen(aux));
  if(position % HBUF_BLOCK == 0)
    h->body = realloc(h->body, h->bodyLength + HBUF_BLOCK);
  h->body[h->bodyLength++] = c;
}

static void addCharToBoundary(HeaderParser hp, char c) {
//char aux[400] = {0};
//sprintf(aux, "BoundaryLength: %d (%p); char: %c", hp->nextBoundary->length, hp->nextBoundary, c);
//write(STDOUT, aux, strlen(aux));
  Boundary b = hp->nextBoundary;
  if(b->length % HBUF_BLOCK == 0) // valgrind
    b->bnd = realloc(b->bnd, b->length + HBUF_BLOCK);
  b->bnd[b->length++] = c;
}

static bool checkIsRelevantHeader(Level current, char c) {
  int index;
  bool ret = FALSE;
  HeaderParser hp = current->headerParser;
  for(index = 0; index < RELEVANT_HEADERS; index++) {
    //printf("potentialRelevantHeader antes: %d\n", hp->potentialRelevantHeader[index]);
    if(hp->potentialRelevantHeader[index] == TRUE) {
      hp->potentialRelevantHeader[index] = cmp(relevantHeaderNames[index], hp->hbufIndex, c);
    }
    //printf("potentialRelevantHeader dsps: %d\n", hp->potentialRelevantHeader[index]);
    if(hp->potentialRelevantHeader[index] == TRUE)
      ret = TRUE;
  }
  return ret;
}

static bool checkIsRelevantMime(char * potentialRelevantMime, int position, char c) {
  int index;
  bool ret = FALSE;
//char auxi[400];
//sprintf(auxi, "El for lo del medio queda %d", relevantMimes - COMPOSITE_MIMES);
//write(STDOUT, auxi, strlen(auxi));
  for(index = 0; index < relevantMimes - COMPOSITE_MIMES; index++) {
   char aux[400] = {0};
//sprintf(aux, "potentialRelevantMIME antes: %d, position: %d, char: %c\n",potentialRelevantMime[index], position, c);
//write(STDOUT, aux, strlen(aux));
//printf("potentialRelevantMIME antes: %d, position: %d, char: %c\n",potentialRelevantMime[index], position, c);
//sprintf(aux, "En isRelevantMime con potentialRelevantMime[index] %c; %s %d vs %d, %c vs %c\n", potentialRelevantMime[index] + '0', censoredMimes[index], position, strlen(censoredMimes[index]), c, censoredMimes[index][position]);
//write(STDOUT, aux, strlen(aux));
    if(potentialRelevantMime[index] == TRUE) {
      potentialRelevantMime[index] = cmpMime(censoredMimes[index], position, c);
    }
//printf("potentialRelevantMIME Despues: %d, position: %d, char: %c\n",potentialRelevantMime[index], position, c);
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
///char aux[400] = {0};
//sprintf(aux, "En cmpMime %s %d vs %d, %c vs %c\n", str, position, strlen(str), c, str[position]);
//write(STDOUT, aux, strlen(aux));
  if(position >= len - 1 && str[len - 1] == '*') {
    //printf("Me dio *************************** %c\n", c);
    return TRUE;
  }
  if(position > len) {
    return FALSE;
  }
  //printf("Voy a retornar %d\n", (tolower(c) == tolower(str[position])) ? 1 : 0);
  return tolower(c) == tolower(str[position]);
}

static bool anyCensoredMatches(const char * potentialRelevantMime, int position) {
  int index;
  for(index = 0; index < relevantMimes - COMPOSITE_MIMES; index++) {
    //printf("anyCensoredMatches %s, position: %d\n", censoredMimes[index], position);
    if(potentialRelevantMime[index] == TRUE && cmpMime(censoredMimes[index], position, '\0'))
      return TRUE;
  }
  return FALSE;
}

static bool isComposite(char potentiallyComposite, int position, int mime) {
  //printf("isComposite, potentialMultipart: %d, position: %d, MIME: %d\n", potentialMultipart, position, mime);
  return potentiallyComposite && cmpMime(compositeMimes[mime], position, '\0');
}

static bool cmp(const char * str, int position, char c) {
  //printf("cmp, %s %d %c vs %c\n", str, position, c, str[position]);
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

// NOTE: The CRLF preceding the encapsulation line is considered part of the boundary 
// so that it is possible to have a part that does not end with a CRLF (line break). 
// Body parts that must be considered to end with line breaks, therefore, should have
// two CRLFs preceding the encapsulation line, the first of which is part of the 
// preceding body part, and the second of which is part of the encapsulation boundary.
static void parseBody(Level current, char c, Boundary boundary) {
  if(boundary == NULL) {
    if(buffer->i == buffer->bytesRead - 1
      || (current->mustBeCensored && buffer->i == buffer->bytesRead - 1)) {
      current->task = DONE_FINAL_BODY;
      //printf("Llegue a parseBody con este char %c y el index es: %d\n", c, buffer->bytesRead);
    //char aux[400] = {0};
    //sprintf(aux, "Buffer->doneReading: %d\n", buffer->doneReading);
    //write(STDOUT, aux, strlen(aux));
      current->taskStatus = WRITING_NO_CHECK;
      consumeRemainingBytes(current);
      return;
    }
    return;
  }
  //char aux[4000] = {0};
  //sprintf(aux, "%s; Char: %c\n", bodyStatusNames[current->taskStatus], c);
  //write(STDOUT, aux, strlen(aux));
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
                                      //printf("boundary[i]: %c\n", boundary->bnd[current->bodyParser->boundaryIndex]);
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
                                        //return;
                                      }
                                      if(c == '-') {
                                        current->bodyParser->hyphenCount++;
                                        if(current->bodyParser->hyphenCount < HYPHENS)
                                          break;
                                      //  printf("HyphenCount: %d\n", current->bodyParser->hyphenCount);
                                      }
                                      if(current->bodyParser->hyphenCount == HYPHENS) {
                                        //printf("POPIE %s\n", boundary->bnd);
                                //write(STDOUT, "ENTRE PARA POPIAR", strlen("entre para popiar"));
                                        current->bodyParser->isFinalPart = TRUE;
                                        Boundary popped = pop(boundaries);
                                        if(current->mustBeCensored)
                                          writeBoundary(current->bodyParser->isFinalPart, boundary);
                                        free(popped->bnd);
                                        free(popped);
      //VERRRRRRRRRRRRRRRRRRRcurrent->task = PARSING_BODY;
                                        current->task = PARSING_BODY;
                                        current->taskStatus = NEW_LINE_BODY;
                                        resetBodyParserSameStartingPoint(current->bodyParser);
                                        break;
                                      }
                                      if(current->mustBeCensored) {
                                        //printf("VOY A ESCRIBIR EL BOUNDARYYYYYYYYYYYYYYYYYYYYY\n");
                                 //write(STDOUT, "NO POPEO", strlen("NO POPEO"));
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
