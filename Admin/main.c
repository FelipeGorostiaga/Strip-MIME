#include "main.h"

struct sockaddr_in addr;


//TODO
//Checkear si con readFromProxy() devolviendo la response null-terminated el flujo no deberia cambiar
//Test socket connections

int main(void) {
    int socket;
    int option = CONTINUE; 

    //testParseMetrics();
    //testSendMetrics();
    //test2();
    //test();
    
    initSettings();
    //socket = connectSocket();
    //authenticate(socket);
    welcome();

    while(option ==  CONTINUE) {
        option = readCommands(socket);
        printf("Option return value: %d\n",option);
    }

    close(socket);
    printf("Closing program...\n");

    return 0;
}

void clearScreen() {
    system("clear");
}

void exitWmsg(char * msg) {
    clearScreen();
    printf("%s\n",msg);
    printf("Exiting...\n");
    sleep(2);
    exit(0);
}

void authenticate(int fd) {
    char * pass;
    char * response;
    int count = 0;
    int scmp = 0;
    char * passMsg = ">Enter secret password: (\"password\")\n";
    char * errorMsg = "Too many incorrect passwords!\n";
    do{ 
        if(scmp != 0) {
            clearScreen();
            count++;
            if(count >= 10) exitWmsg(errorMsg);
            printf("Invalid password, try again...\n");
        } 
        pass = getpass(passMsg);
        if(sendToProxy('x',pass,strlen(pass),fd)) {
            response = (char *)readFromProxy(fd,'x');
            scmp = strcmp(response,"OK");
        }
    }while(scmp != 0);
    free(response);
    fflush(stdin);
    clearScreen();
    printf("Signing in...\n");
    sleep(2);
    clearScreen();
}

void printHelp() {
    clearScreen();
    printf("HELP MENU\n");
    printf("Write one command per line\n");
    printf("Available commands:\n");
    printf("-f <filename> Redirect metrics to filename\n");
    printf("-t <command> Configure proxy server to execute command on incoming mails from origin server\n");
    printf("-e <errorfilename> Redirect stderr to errorfilename \n");
    printf("-m <message> change default replace message\n");
    printf("-v Get proxy server version\n");
    printf("-z BYTES CON_CON TOT_CON TIME Get specified metrics, no parameters will get all metrics\n");   
}

void welcome() {
    printf("Welcome Administrator!\n");
    printf("Please insert command followed by <ENTER> \n");
    printf("Type -h for help menu \n");
}

void printErrorMessage(int errorCode) {
    switch(errorCode) {
        case 0: printf("Invalid format command.\n");break;
        case 1: printf("Invalid command, missing arguments.\n");break;
        case 2: printf("Invalid command, too many blank spaces.\n");break;
        case 3: printf("Invalid command, parameters can´t exceed 1024 characters long.\n");break;
        case 4: printf("Error sending data to server.\n");break;
        case 5: printf("Error, invalid response from server, please try again...\n");break;
        case 6: printf("Server responded with ERROR\n");break;
        case 7: printf("Invalid metric parameters, please try again...\n");break;
        case 8: printf("Error, couldn't read from server.\n");break;
        case 9: printf("Invalid metric values from server.\n");break;
    }
}

void successMessage(int cmd) {
    switch(cmd) {
        case 'm': printf("Successfully updated replace message\n");break;
        case 't': printf("Successfully updated command\n");break;
        case 'e': printf("Successfully updated error file redirect\n");break;
        case 'f': printf("Successfully updated metrics file redirect\n");break;
    }

}

int readCommands(int socket) {
    printf("en read commands\n");
    char * buff = (char *)malloc(BUFF_SIZE * sizeof(char));
    int errorNum;
    int spaceCount = 0;
    enum states state = START;
    int c,cmd,rt;

    while(state != END && (c = getchar()) != EOF &&  c != '\n' ) {

        switch(state) {

            case START:
                if (c == '-' ) state = GUION;
                else if (isspace(c)) {
                    state = SPACE;
                }
                else {
                    state = ERROR;
                    errorNum = 0;
                }
                break;

            case SPACE:
                if(isspace(c)) {
                    if(spaceCount < MAXSPACES)
                        spaceCount++;
                    else
                        state = ERROR;
                        errorNum = 2;
                }
                else if (c == '-')
                    state = GUION;
                else {
                    state = ERROR;
                    errorNum = 0;
                }
                break;

            case ERROR:
                cleanBuffer();
                fflush(stdin);
                printErrorMessage(errorNum);
                state = END;
                break;

            case VERSION:
                if(!isspace(c)) {
                    state = ERROR;
                    errorNum = 0;
                }
                else {
                    getProxyVersion();
                    state = END;
                }
                break;
            case HELP:
                if(isspace(c)) {
                    printHelp();
                    state = END;
                }
                else {
                    state = ERROR;
                    errorNum = 1;
                }
                break;
            case LETTER:
                if(isspace(c)) {
                    state = WORD;
                }
                else {
                    state = ERROR;
                    errorNum = 0;
                }    
                break;
            case WORD:
                buff[0] = (char)c;
                rt = getLine(NULL,buff+1, BUFF_SIZE);
                if(rt == NO_INPUT ) {
                    state = ERROR;
                    errorNum = 1;
                }
                else if(rt == TOO_LONG) {
                    state = ERROR;
                    errorNum = 3;
                }
                else {
                    handleCommandProxy(buff,cmd,socket);
                    state = END;
                }

                break;
            case QUIT:
                if(!isspace(c)) {
                    state = ERROR;
                    errorNum = 0;
                }
                else {
                    return 0;
                }
            case GUION:
                if(!(isalpha(c))) {
                    state = ERROR;
                    errorNum = 0;
                }
                else {
                    if(c == 'h')state = HELP;
                    else if(c ==  'v') state = VERSION;
                    else if(c == 't' || c == 'e' || c == 'f' || c == 'm' || c == 'z' ) {
                        state = LETTER;
                        cmd = c;
                    }
                    else if(c == 'q') state = QUIT;
                    else {
                        state = ERROR;
                        errorNum = 0;
                    }
                }
                break;

        }
    }
    free(buff);

    if(state == QUIT) {
        printf("Closing program...\n");
        fclose(stdout);
        sleep(2);
        return 0;
    }
    else if (state == END) {
        printf("Ended in END state, try again...\n");
    }
    else if(state == ERROR) {
        printErrorMessage(errorNum);
    }
    else if(state == VERSION) getProxyVersion();
    else if(state == HELP) printHelp();
 
    return 1;
}

void cleanBuffer() {
    int c;
    fflush(stdin);
    while(c=getchar() != '\n');
}

void getProxyVersion(int cmd, int socket) {
    int rt;
    char * response;
    char * buffer = malloc(sizeof(char));
    
    rt = sendToProxy(cmd,buffer,0,socket);
    if(!rt) {
        printErrorMessage(4);
    }
    response = readFromProxy(socket,cmd);
    clearScreen();
    printf("Proxy Version %s\n",response);
    free(buffer);
    free(response);
    return;
}
 
void parseMetrics(char * response,int count) {
    uint8_t size;
    char c;
    char * metric;
    char * metricValue;

    if(response != NULL ) {
        if(response[0] == 'z') {
            if((size = (uint8_t)response[1]) > 0) {
                printf("size %d\n",(int)size);

                if (isErrorMessage(response + (char)1)) {
                    printErrorMessage(6);
                    return;
                }   
                metric = strtok((char *)(response + (char)1)," ");
                metricValue = strtok(NULL," ");
                printMetric(metric,metricValue);
                count--;
                
                while(metric != NULL && metricValue != NULL && count > 0) {
                    metric = strtok(NULL," ");
                    metricValue = strtok(NULL," ");
                    printMetric(metric,metricValue);
                    count--;
                }
            }  
        }
    }
    else {
        printErrorMessage(5);
    }
    return;
}

int sendMetrics(char * buff, int fd) {
    int countMetrics = 0;
    char * createdBuffer = (char *)malloc(200);
    char *metrics[4] = {"BYTES","CON_CON","TOT_CON","CON_PER_MIN"};
    int sizeMetrics[4] = {5,7,7,11};
    int size = 0;
   
    for(int i=0 ; i<4 ; i++) {
        if(strstr(buff,metrics[i]) != NULL) {
            printf("Metric %s FOUND\n",metrics[i]);
            
            if(countMetrics > 0){ 
                createdBuffer[size] = ' ';
                size++;
            }
            strcpy((char *)(createdBuffer +(char)size),metrics[i]);
            size += sizeMetrics[i];
            countMetrics++;
        }
    }
    if(countMetrics > 0) {
        if(!(sendToProxy('z',createdBuffer,size,fd))) {
            printErrorMessage(4);
            return 0;
        }
    }
    else {
        printErrorMessage(7);
    }
    free(createdBuffer);
    return countMetrics;
}

void readResponse(char * response, int cmd) {

    uint8_t size;
    char c;
    int error = 0;
    if(response != NULL ) {
        c = response[0];
        if(c == cmd) {
            size = response[1];
            if(size == 2) {
                if(isOK(response + 2)) {
                    successMessage(cmd);
                }
            }
            else {
                error = TRUE;
            }
        }
        else {
            error = TRUE;
        }
    }
    else {
        error = TRUE;
    }
    if(error) printErrorMessage(5);

}

int isOK(char *s) {
    return (*s == 'O' && (*(s+1)) == 'K'); 
}


int handleCommandProxy(char *buff, int cmd, int fd) {
    int rt;
    printf("in handleCommandProxy\n");
    char * response;

    printf("Command value %c\n",cmd);
    printf("parameter value:%s\n",buff);
    printf("parameter length:%d\n",(int)strlen(buff));

    if(cmd == 'z') {
        rt = sendMetrics(buff,fd);
        if(rt) {
            response = readFromProxy(fd,cmd);
            parseMetrics(response,rt);
        }        
    }
    else {

        rt = sendToProxy(cmd,buff,strlen(buff),fd);
        if(rt) {
            response = readFromProxy(fd,cmd);
            readResponse(response,cmd);
        }
        else {
            printErrorMessage(4);
        }
    }
}
//Retorna el string NULL-TERMINATED
char * readFromProxy(int fd, int cmd) {
    char b[1];
    char *buff = malloc(257 * sizeof(char));
    uint8_t readSize;
    if(read(fd,b,1) != 1) {
        printErrorMessage(8);
        return NULL;
    }
    else {
        if(b[0] == (char)cmd) {
            if(read(fd,b,1) != 1) {
                printErrorMessage(5);
                return NULL;
            }
            readSize = (uint8_t)b[0];
        }
        else {
            printErrorMessage(5);
            return NULL;
        }
    }
    if(readSize > 255) {
        printErrorMessage(5);
        return NULL;
    }
    read(fd,buff,readSize);
    buff[readSize] = '\0';
    printf("String recieved:%s\n",buff);
    return buff;
}
int sendToProxy(int cmd, char * buffer, size_t size, int fd) {

    char sendSize;
    char sendBuffer[257];
    int index = 0;
    int actSize;
    do {
        setToZero(sendBuffer,sizeof(sendBuffer));
        if(size > 255) sendSize = 255;
        else sendSize = size;
        size -= sendSize;
        sendBuffer[0] = (char)cmd;
        sendBuffer[1] = (char)sendSize;
        memcpy(sendBuffer + 2, buffer + index, sendSize);
        actSize = sendSize + 2;
        if( write(fd,sendBuffer,actSize) !=  actSize) {
            return 0;
        }
        index += sendSize;
    }while(size != 0);

    return 1;
}

void removeSpaces(char * source) {
  char* i = source;
  char* j = source;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = 0;
}

void testSendMetrics() {

    char * rt;
    char * buff = (char *)malloc(200);
    char * createdBuffer = (char *)malloc(200);
    char *metrics[4] = {"BYTES","CON_CON","TOT_CON","CON_PER_MIN"};
    int sizeMetrics[4] = {5,7,7,11};
    int countMetrics = 0;
    int size = 0;

    getLine("Insert test string\n",buff,200);
    printf("Entered string:%s\n",buff);

    for(int i=0 ; i<4 ; i++) {
        if(strstr(buff,metrics[i]) != NULL) {
            printf("Metric %s FOUND\n",metrics[i]);
            
            if(countMetrics > 0){ 
                createdBuffer[size] = ' ';
                size++;
            }
            strcpy((char *)(createdBuffer +(char)size),metrics[i]);
            size += sizeMetrics[i];
            countMetrics++;
        }
    }
    //createdBuffer[size] = '\0';
    printf("Count metrics:%d\nSize:%d\n",countMetrics,size);
    printf("Created buffer:%s\n",createdBuffer);
    free(buff);
    free(createdBuffer);

}

void tokenizeMetrics(char *buff) {
    char * token;
    int i = 0;
    int errorFlag = 0;
    char params[3][40];

    token = strtok(buff," ");
    printf("token:%s\n",token);
    strcpy(params[i],token);

    while((token = strtok(NULL," ")) != NULL) {
        i++;
        if(i > 2){
            errorFlag = 1;
            break;
        }
        printf("token:%s\n",token);
        strcpy(params[i],token);
    }
    printf("%d\n",i);
    if(errorFlag) {
        printf("Too many arguments\n");
    }
    else { 
        for(int j = 0 ; j<=i ; j++) {
            printf("params[%d]=%s\n",j,params[j]);
        }
    }
}

void test() {
    char * rt;
    char buff[100];
    getLine("Insert test string\n",buff,sizeof(buff));
    printf("Entered string:%s\n",buff);
    char *str[4] = {" uno "," dos "," tres "," cuatro "};
    for(int i = 0 ; i<4 ; i++) {
        if((rt = strstr(buff,str[i])) != NULL) {
            printf("%s esta! \n",str[i]);
        }
    }

}

void test2() {
    char * rt;
    char buff[100];
    getLine("Insert test string\n",buff,sizeof(buff));
    printf("Entered string:%s\n",buff);
    removeSpaces(buff);
    printf("No spaces =%s\n",buff);
}

int getLine(char *prmpt, char *buff, size_t sz) {
    int ch, extra;

    if (prmpt != NULL) {
        printf ("%s", prmpt);
        fflush (stdout);
    }
    if (fgets (buff, sz, stdin) == NULL)
        return NO_INPUT;

    if (buff[strlen(buff)-1] != '\n') {
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? TOO_LONG : OK;
    }
    buff[strlen(buff)-1] = '\0';
    return OK;
}



void initSettings() {
    memset((void*)&addr,0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

int connectSocket() {
    int fd, ret;
    struct sctp_initmsg initmsg;
    struct sctp_event_subscribe events;

    if((fd = socket(AF_UNSPEC,SOCK_STREAM,IPPROTO_SCTP)) == -1 ) {
        printf("Error creating socket\n");
        exit(1);
    }

    memset(&initmsg,0, sizeof(struct sctp_initmsg));
    initmsg.sinit_num_ostreams  = MAX_STREAMS;
    initmsg.sinit_max_instreams = MAX_STREAMS;
    initmsg.sinit_max_attempts  = MAX_STREAMS;
    ret = setsockopt(fd,IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(struct sctp_initmsg));
    if(ret<0) {
        perror("setsockopt SCTP_INITMSG");
        exit(1);
    }

    events.sctp_association_event = 1;
    events.sctp_data_io_event = 1;
    ret = setsockopt(fd,IPPROTO_SCTP,SCTP_EVENTS, &events, sizeof(events));
    if(ret < 0) {
        perror("setsockopt SCTP_EVENTS");
        exit(1);
    }
 
    if((ret = connect(fd,(struct sockaddr*)&addr, sizeof(addr))) == -1) {
        printf(("Error connecting to proxy socket\n"));
        exit(1);
    }
    return fd;
}

void setToZero(char *buff, size_t size) {
    memset(buff,0,size);
}


void testParseMetrics() {
    int count = 2;
    char * buff = (char *)malloc(200);
    getLine("Insert test string\n",buff,200);
    printf("Entered string:%s\n",buff);
    parseMetrics(buff,count);
}

void printMetric(char * metric, char * metricValue) {
    int value;
    if(metric == NULL || metricValue == NULL) {
        printErrorMessage(9);
    }
    else {
        value = atoi(metricValue);
        printf("Metric:%s with value:%d\n",metric,value);
    }
    return;
}

int countDigits(char *str) {
    int count = 0;
    while(isdigit(str[count])) {
        count++;
    }
    return count + 1;
}

int isErrorMessage(char * string) {
    char * s = malloc(6);
    int res;
    memcpy(s,string,5);
    s[5] = '\0';
    res = strcmp(s,"ERROR");
    free(s);
    if(s == 0) return 1;
    return 0;
}