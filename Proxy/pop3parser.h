//
// Created by sswinnen on 17/10/18.
//

#include <unistd.h>
#include <stdio.h>
#define TRUE 1
#define FALSE 0

void readPop3FromClient(int clientFd, int originServer);
void readPop3FromOrigin(int originServerFd);
