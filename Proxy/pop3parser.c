//
// Created by sswinnen on 17/10/18.
//

#include "pop3parser.h"

void readPop3FromClient(int clientFd, int originServer) {
    char c;
    int r = FALSE;
    int finish = FALSE;

    while(!finish) {
        read(clientFd,&c, 1);
        if(c == '\r') {
            r = TRUE;
        } else if(c == '\n' && r == TRUE) {
            finish = TRUE;
        } else {
            r = FALSE;
        }
        write(originServer,&c,1);
    }
}

void readPop3FromOrigin(int originServerFd) {
    char c;
    int r = FALSE;
    int finish = FALSE;
    //TODO pasarle todo al ejecutable que filtra
    while(!finish) {
        read(originServerFd,&c, 1);
        if(c == '\r') {
            r = TRUE;
        } else if(c == '\n' && r == TRUE) {
            finish = TRUE;
        } else {
            r = FALSE;
        }
        printf("%c",c);
    }
}