#ifndef BACKUP_H_
#define BACKUP_H_

// void *primary_healthcheck();
void connect_to_primary(char *ip, list_t **other_processes, process_t *self);
void get_other_processes_data(list_t **other_processes, process_t *self);
void connect_to_others(char **argv, list_t **other_processes, process_t *self);

#endif
