#ifndef LEADER_H_
#define LEADER_H_

#define PRIMARY 0
#define BACKUP 1

#define DEFAULT_PORT 4000

#define MISSING_PARAMETER_FOR_BACKUP_PROCESS "Processos de backup devem informar o ip do processo primário como terceiro parâmetro.\nExemplo de uso:\n\n\t$./leader backup 127.0.0.1 5000\n\n"
#define MISSING_PARAMETER "Parâmetro faltando.\nUso correto:\n\n\t$./leader primary\n\tou\n\t$./leader backup [ip do processo primário] [porta para conectar ao primário]\n\n"
#define INVALID_PARAMETER "Parâmetro não reconhecido.\nValores possíveis:\n\n\tprimary\n\tbackup\n\n"

typedef struct process {
  int pid;
  char *ip;
  int role;
  int socket_id;
  int port;
  struct sockaddr_in address;
} process_t;

void print_processes_list();
void receive_other_processes_data_from_primary(process_t *primary);

#endif
