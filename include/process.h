#ifndef PROCESS_H_
#define PROCESS_H_

#define PRIMARY 0
#define BACKUP 1

#define DEFAULT_PORT 5000
#define IP_STRING_LENGTH 30
#define HEALTHCHECK_FREQUENCY 4
#define HEALTHCHECK_TIMEOUT 2

typedef struct process {
  int pid;
  char ip[IP_STRING_LENGTH];
  int role;
  int socket_id;
  int port;
  struct sockaddr_in address;
} process_t;

#endif
