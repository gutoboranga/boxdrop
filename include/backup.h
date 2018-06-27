#ifndef BACKUP_H_
#define BACKUP_H_

#include <process.h>

// void *primary_healthcheck();
void connect_to_primary(char *ip, list_t **other_processes, process_t *self);
void get_other_processes_data(list_t **other_processes, process_t *self);
void connect_to_others(char **argv, list_t **other_processes, process_t *self);
process_t *get_process_from_pid(list_t **other_processes, int pid);
list_t *list_remove_with_pid(list_t *head, int pid);

#endif
