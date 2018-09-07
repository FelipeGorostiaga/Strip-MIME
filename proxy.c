//
// Created by sswinnen on 07/09/18.
//

#include <stdlib.h>
#include "proxy.h"
#define SOCKET_BLOCK 10


int main(void) {

    int i = 0;
    int * clientSockets = malloc(SOCKET_BLOCK*sizeof(int));
    

    while(1) {
        /*Accept new clients NONBLOCKING!!!!!!! (fork discarded)*/
        /*Check for incoming data from sockets using select*/
        /*If found, send incoming data from web to external parser (call to match.c) and get response*/
        /*Send parsed data either to client or to internet, open sockets to other internet servers*/
    }
}