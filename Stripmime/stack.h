#include <stdio.h>
#include <stdlib.h>

typedef struct ElementCDT * Element;

typedef struct ElementCDT {
  void * content;
  Element next;
} ElementCDT;

typedef struct StackCDT * Stack;

typedef struct StackCDT {
  int size;
  Element current;
} StackCDT;

Stack newStack();

void * pop(Stack stack);

void * peek(Stack stack);

int push(Stack stack, void * content);
