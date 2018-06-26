#ifndef LIST_H_
#define LIST_H_

typedef struct list {
	void *value;
	struct list *next;
} list_t;


list_t* list_new(void);
list_t* list_make_node(void *value);
list_t* list_free(list_t *list);
void list_insert(list_t **list, void *content);
void list_insert_item(list_t *first, list_t *item);
list_t *list_invert(list_t *list);
//list_t *list_remove_with_pid(list_t *head, int pid);

#endif
