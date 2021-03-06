#include <stdio.h>
#include <stdlib.h>
#include "list.h"

list_t* list_new(void) {
    return NULL;
}

list_t* list_make_node(void *value) {
  list_t *node = malloc(sizeof(list_t));
	if (!node)
		printf("Failed to allocate memory for list node");

	node->value = value;
	node->next = NULL;
	
	return node;
}

list_t *list_free(list_t *list) {

  list_t *aux;

  while (list != NULL) {
      aux = list;
      list = list->next;
      printf("tirando o %s\n", ((char*)aux->value));
      free(aux);
  }

  return NULL;
}

void list_insert_item(list_t *first, list_t *item) {
  list_t *aux;
  aux = first;
  
  while (aux->next != NULL) {
    aux = aux->next;
  }
  
  aux->next = item;
}

void list_insert(list_t **list, void *content) {

  list_t *aux;

  if (*list == NULL) {
    *list = malloc(sizeof(list_t));
    (*list)->value = content;
    (*list)->next = NULL;
  }

  else {
    aux = *list;

    while (aux->next != NULL) {
      aux = aux->next;
    }

    aux->next = malloc(sizeof(list_t));
    aux->next->value = (void*)content;
    aux->next->next = NULL;
  }
}

list_t *list_invert(list_t *head) {
  list_t *next, *current, *previous;
  
  current = head;
  previous = NULL;
  next = NULL;
  
  while (current != NULL) {
    next = current->next;
    current->next = previous;

    previous = current;
    current = next;
  }
  
  return previous;
}



// int main(int argc, char **argv) {
//   list_t *head, *e1, *e2, *e3;
//
//   head = list_make_node((void *) 6);
//   e1 = list_make_node((void *) 1);
//   e2 = list_make_node((void *) 209);
//   e3 = list_make_node((void *) 42);
//
//   list_insert_item(head, e1);
//   list_insert_item(head, e2);
//   list_insert_item(head, e3);
//
//   head = list_remove_with_pid(head, atoi(argv[1]));
//
//   list_print(head);
//
//   return 0;
// }
