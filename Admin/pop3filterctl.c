#include "pop3filterctl.h"

int main(int argc, char * argv []) {
    int socket;
    int option = CONTINUE;
    int port;
    char * address;

    if(argc == 1) {
        address = "127.0.0.1";
        port = 9090;
    }
    else {
        address = argv[1];
        port = atoi(argv[2]);
    }
    socket = connectSocket(address, port);
    authenticate(socket);
    welcome();
    while(option ==  CONTINUE) {
        option = readCommands(socket);
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
    char * response = NULL;
    int count = 0;
    int scmp = 1;
    char * passMsg = "Enter secret password:\n>";
    char * errorMsg = "Too many incorrect passwords!\n";
    do{ 
        pass = getpass(passMsg);
        printf("PASS: %s\n",pass);
        if(sendToProxy('x',pass,strlen(pass),fd)) {
            response = readFromProxy(fd,'x');
            printf("Response to password: %s\n", response);
            if(response != NULL) {
                scmp = strcmp(response,"OK");
                free(response);
            }
        }
        if(scmp != 0) {
            clearScreen();
            count++;
            if(count >= 10) exitWmsg(errorMsg);
            printf("Invalid password, try again...\n");
        } 
    } while(scmp != 0);
    fflush(stdin);
    clearScreen();
    printf("Signing in...\n");
    sleep(2);
    clearScreen();
}


int readCommands(int socket) {
    printf("Enter command:\n");
    char * buff = (char *)malloc(BUFF_SIZE * sizeof(char));
    int errorNum = 0;
    int spaceCount = 0;
    enum states state = START;
    int c = 0,cmd = 0,rt = 0;

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
                    if(spaceCount < MAXSPACES) {
                        spaceCount++;
                    }
                    else {
                        state = ERROR;
                        errorNum = 2;
                    }

                }
                else if (c == '-') {
                    state = GUION;
                }
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
                    getProxyVersion('v',socket);
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
                    handleCommandProxy(buff,cmd,socket,0);
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
                break;

            case GUION:
                if(!(isalpha(c))) {
                    state = ERROR;
                    errorNum = 0;
                }
                else {
                    if(c == 'h')state = HELP;
                    else if(c ==  'v') state = VERSION;
                    else if(c == 't' || c == 'e' || c == 'f' || c == 'm' || c == 'z' || c == 'M') {
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
            case END: break;
        }
    }
    free(buff);

    if(state == QUIT) {
        printf("Closing program...\n");
        fclose(stdout);
        sleep(2);
        return 0;
    }
    else if(state == LETTER && cmd == 'z') handleCommandProxy(buff,cmd,socket,1);
    else if(state == WORD) printErrorMessage(0);
    else if(state == ERROR) printErrorMessage(errorNum);
    else if(state == LETTER) printErrorMessage(1); 
    else if(state == VERSION) getProxyVersion('v',socket);
    else if(state == HELP) printHelp();
 
    return 1;
}

void cleanBuffer() {
    fflush(stdin);
    while(getchar() != '\n');
}

void getProxyVersion(int cmd, int socket) {
    int rt;
    char * response;
    char * buffer = malloc(sizeof(char));
    clearScreen();
    rt = sendToProxy(cmd,buffer,0,socket);
    if(!rt) {
        printErrorMessage(4);
    }
    response = readFromProxy(socket,cmd);
    clearScreen();
    printf("Proxy Version %s\n",response);
    free(buffer);
    free(response);
}
 
void parseMetrics(char * response,int count) {
    char * metric;
    char * metricValue;

    if(response != NULL ) {
        if (isErrorMessage(response)) {
            printErrorMessage(6);
            return;
        }
        printf("Data from Server:\n");   
        metric = strtok(response," ");
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
    
    else {
        printErrorMessage(5);
    }
}

int sendMetrics(char * buff, int fd) {
    int countMetrics = 0;
    char * createdBuffer = (char *)malloc(200);
    char *metrics[4] = {"BYTES","CON_CON","TOT_CON","CON_PER_MIN"};
    int sizeMetrics[4] = {5,7,7,11};
    int size = 0;
    setToZero(createdBuffer,200);
    for(int i=0 ; i<4 ; i++) {
        if(strstr(buff,metrics[i]) != NULL) {
            
            if(countMetrics > 0){ 
                createdBuffer[size] = ' ';
                size++;
            }
            strcpy((createdBuffer +(char)size),metrics[i]);
            size += sizeMetrics[i];
            countMetrics++;
        }
    }
    if(countMetrics > 0) {
        if(!(sendToProxy('z',createdBuffer,(size_t)size,fd))) {
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

    if(response == NULL) {
        printErrorMessage(5);
    }
    if(strcmp(response,"OK") == 0) {
        successMessage(cmd);
    }
    else if (strcmp(response,"ERROR") == 0) {
        if(cmd == 'M') {
            printErrorMessage(9);
        }
        else {
            printErrorMessage(6);
        }
    }
    else {
        printErrorMessage(5);
    }
 }


int isOK(const char *s) {
    return (*s == 'O' && (*(s+1)) == 'K'); 
}

void sendAllMetrics(int fd) {
    char str[] = "BYTES CON_CON TOT_CON";
    sendToProxy('z',str,strlen(str),fd);
}

void handleCommandProxy(char *buff, int cmd, int fd,int def) {
    int rt = 0;
    char * response = NULL;
    clearScreen();

    if(cmd == 'z') {
        if(def) {
            rt = 3;
            sendAllMetrics(fd);
        }
        else {
            rt = sendMetrics(buff,fd);
        }
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
    free(response);
}

char * readFromProxy(int fd, int cmd) {
    char firstBytes[2];
    char * buff = malloc(255 * sizeof(char));

    ssize_t bytesRead;
    
    if(read(fd,firstBytes,2) == -1) {
        printErrorMessage(8);
        return NULL;
    }
    if(firstBytes[0] != (char)cmd ) {
        printErrorMessage(5);
        return NULL;
    }
    if(firstBytes[1] > 255) {
        printErrorMessage(5);
        return NULL;
    }
    if((bytesRead = read(fd,buff,(uint8_t )firstBytes[1])) == -1) {
        return NULL;
    }
    buff[bytesRead] = '\0';
    return buff;
}

int sendToProxy(int cmd, char * buffer, size_t size, int fd) {
    uint8_t sendSize;
    char sendBuffer[257];
    int index = 0;
    size_t actSize;

    do {
        setToZero(sendBuffer,sizeof(sendBuffer));
        if(size > 255) sendSize = 255;
        else sendSize = (uint8_t)size;
        size -= sendSize;
        sendBuffer[0] = (char)cmd;
        sendBuffer[1] = sendSize;

        memcpy(sendBuffer + 2, buffer + index, sendSize);
        actSize = sendSize + 2;
        if(write(fd,sendBuffer,(size_t)actSize) ==  -1) {
            printf("Error, write in send to proxy function failed to send all bytes! \n");
            return 0;
        }
        index += sendSize;
    }while(size != 0);

    return 1;
}

int getLine(char *prmpt, char *buff, size_t sz) {
    int ch, extra;

    if (prmpt != NULL) {
        printf ("%s", prmpt);
        fflush (stdout);
    }
    if (fgets (buff, (int)sz, stdin) == NULL)
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


void setToZero(char *buff, size_t size) {
    memset(buff,0,size);
}

void printMetric(char * metric, char * metricValue) {
    int value;
    if(metric == NULL || metricValue == NULL) {
        printErrorMessage(9);
    }
    else {
        value = atoi(metricValue);
        printf("Metric %s = %d\n",metric,value);
    }
}

int isErrorMessage(char * string) {
    int res;
    char * s = malloc(6);
    memcpy(s,string,5);
    s[5] = '\0';
    res = strcmp(s,"ERROR");
    free(s);
    if(res == 0) return 1;
    return 0;
}

int connectSocket(char * address, int port) {
    int fd;

    if((fd = socket(AF_INET,SOCK_STREAM,IPPROTO_SCTP)) == -1 ) {
        printf("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, address, &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        printf("Error connecting to proxy server\n");
        exit(EXIT_FAILURE);
    }
    return fd;
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
    printf("-M <mimes> select mimes to strip from mails\n");
    printf("-z BYTES CON_CON TOT_CON TIME Get specified metrics, no parameters will get all metrics\n");   
    printf("-q Exit \n");
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
        case 6: printf("Error, invalid filename\n");break;
        case 7: printf("Invalid metric parameters, please try again...\n");break;
        case 8: printf("Error, couldn't read from server.\n");break;
        case 9: printf("Invalid metric values from server.\n");break;
        case 10: printf("Error, invalid mime\n");break;
        default: break;
    }
}

void successMessage(int cmd) {
    switch(cmd) {
        case 'm': printf("Successfully updated replace message\n");break;
        case 't': printf("Successfully updated command\n");break;
        case 'e': printf("Successfully updated error file redirect\n");break;
        case 'f': printf("Successfully updated metrics file redirect\n");break;
        case 'M': printf("Succesfullly updated MIMES\n");break;
        default: break;
    }

}

