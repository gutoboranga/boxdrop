#ifndef PROCESS_H_
#define PROCESS_H_

#include <list.h>

#define PRIMARY 0
#define BACKUP 1

#define DEFAULT_PORT 5000
#define IP_STRING_LENGTH 30
#define HEALTHCHECK_FREQUENCY 4
#define HEALTHCHECK_TIMEOUT 2

#define PORT_OFFSET 1000
#define NULL_SOCKET_ID -1000

list_t *other_processes;

typedef struct process {
  int pid;
  char ip[IP_STRING_LENGTH];
  int role;
  int socket_id;
  int socket_id_2;
  int port;
  struct sockaddr_in address;
  struct sockaddr_in address_2;
} process_t;

#endif
