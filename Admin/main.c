#include "main.h"

struct sockaddr_in addr;

//TODO
// - Send version request and read reply
// - Send command request and read reply
// - Send metrics request and read reply
// - Send password and read reply to authenticate


int main(void) {
    int socket;
    int option = CONTINUE; 
    
    initSettings();
    //socket = connectSocket();
    authenticate(socket);
    welcome();

    while(option ==  CONTINUE) {
        option =readCommands();
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
    //sleep(2);
    exit(0);
}

void authenticate(int fd) {
    char * pass;
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
        //TODO - SEND PASSWORD IN SOCKET AND GET ANSWER FROM PROXY, OK OR ERROR.
        pass = getpass(passMsg);
        scmp = strcmp(pass,"password");
    }while(scmp != 0);

    fflush(STDIN_FILENO);
    clearScreen();
    printf("Signing in...\n");
    sleep(2);
    clearScreen();
}

void checkInput(int ret) {
    if(ret == TOO_LONG) printf("Invalid input, username can't be more than 25 characters long.\n");
    else if(ret == NO_INPUT) printf("Invalid input, please enter a valid username.\n");
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
    printf("-z BYTES CCON CON CTIME Get specified metrics, no parameters will get all metrics\n");   
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
    }
}
void allocMetrics(char **metrics) {

    metrics = malloc(3 * sizeof(char *));
    for(int i = 0 ; i < 3 ; i++) {
        metrics[i] = malloc(40 * sizeof(char));
    }

}

int readCommands() {

    printf("en read commands\n");
    char buff[1024];
    int errorNum;
    int spaceCount = 0;
    enum states state = START;
    int c,cmd,rt;
    char **metrics;
    allocMetrics(metrics);

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
                rt = getLine(NULL,buff+1, sizeof(buff));
                if(rt == NO_INPUT ) {
                    state = ERROR;
                    errorNum = 1;
                }
                else if(rt == TOO_LONG) {
                    state = ERROR;
                    errorNum = 3;
                }
                else {
                    handleCommandProxy(buff,cmd,sizeof(buff),metrics);
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

    if(state == QUIT) {
        printf("Closing program...\n");
        fclose(stdout);
        sleep(2);
        return 0;
    }
    else if (state == END) {
        printf("\n\n");
        printf("Ended in END state, try again...\n");
    }
    else if(state == ERROR) {
        printErrorMessage(errorNum);
    }
    else if(state == VERSION) getProxyVersion();
    else if(state == HELP) printHelp();
 
    return 1;
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

void cleanBuffer() {
    int c;
    fflush(stdin);
    while(c=getchar() != '\n');
}

void getProxyVersion() {
    clearScreen();
    printf("Proxy version 1.0.0\n");
    return;
}

int handleCommandProxy(char *buff, int cmd, size_t  buffSize , char **metrics) {
    int rt;
    char sendbuff[257];
    printf("in handleCommandProxy\n");
    if(cmd == 'z') {
        getMetrics(buff,metrics);
        
    }
    else {
        fillBuffer(sendbuff,buff,cmd);       
        printf("Command value %c\n",cmd);
        printf("parameter value: %s\n",buff);
        printf("parameter length: %d\n",(int)strlen(buff));
    }

}

void fillBuffer(char * sendbuff, char * buff, int cmd) {
    sendbuff[0] = (char)cmd;
    int len = strlen(buff);
    sendbuff[1] = (char)len;
}

void createProtocolPackage(int command, char * buff, size_t buffsize) {
    return;
}

void getMetrics(char * buff, char ** metrics) {
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
        
    return;
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
