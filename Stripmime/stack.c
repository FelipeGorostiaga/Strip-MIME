#include "stack.h"

Stack newStack() {
  Stack stack = malloc(sizeof(StackCDT));
  stack->size = 0;
  stack->current = NULL;
  return stack;
}

int push(Stack stack, void * content) {
  if(stack == NULL || content == NULL)
    return -1;
  Element aux = stack->current;
  stack->current = malloc(sizeof(ElementCDT));
  stack->current->content = content;
  stack->current->next = aux;
  stack->size++;
  return stack->size;
}

void * peek(Stack stack) {
  if(stack == NULL || stack->current == NULL)
    return NULL;
  return stack->current->content;
}

void * pop(Stack stack) {
  if(stack == NULL || stack->current == NULL)
    return NULL;
  Element aux = stack->current;
  void * ret = stack->current->content;
  stack->current = stack->current->next;
  free(aux);
  stack->size--;
  return ret;
}
