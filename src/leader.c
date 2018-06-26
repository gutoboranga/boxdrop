#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <dropboxUtil.h>
#include <sys/stat.h>
#include <leader.h>
#include <list.h>
#include <backup.h>

process_t self;
list_t *other_processes;

int internal_port = DEFAULT_PORT;
int internal_socket_id;

void init(int argc, char **argv) {
  get_local_ip(self.ip);
  self.port = internal_port;
  self.pid = getpid();
  
  // handle process role input
  if (strcmp(argv[1], "primary") == 0) {
    self.role = PRIMARY;
  } else if (strcmp(argv[1], "backup") == 0) {
    self.role = BACKUP;
    
    // se estiver faltando argumentos
    if (argc < 4) {
      printf(MISSING_PARAMETER_FOR_BACKUP_PROCESS);
      exit(1);
    }
    
    // salva a porta especificada
    self.port = atoi(argv[3]);
    
  } else {
    printf(INVALID_PARAMETER);
    exit(1);
  }
}

char *serialize_process(process_t *p) {
  char *buffer = malloc(sizeof(char) * MAX_PACKAGE_DATA_LENGTH);
  memcpy(buffer, &p, sizeof(process_t));
  
  return buffer;
}


void print_processes_list() {
  list_t *aux = other_processes;
  printf("-- Outros processos --\n");
  if (aux == NULL) {
    printf("lista vazia ...\n");
  }
  while (aux != NULL) {
    process_t *p = (process_t *) aux->value;
    
    printf("pid: %d\trole: %s\n", p->pid, p->role == 0 ? "primário" : "backup");
    
    aux = aux->next;
  }
  printf("----------------------\n\n");
}

process_t *get_process_from_message(message_t *msg, struct sockaddr_in cli_addr) {
  // de-serializa dados do novo processo, que vieram junto na mensagem
  process_t *new_process = malloc(sizeof(process_t));
  memcpy(new_process, msg->data, sizeof(process_t));
  
  // salva o endereço do novo processo
  char *ip_string = inet_ntoa(new_process->address.sin_addr);
  memcpy(&new_process->address, &cli_addr, sizeof(cli_addr));
  strcpy(new_process->ip, inet_ntoa(new_process->address.sin_addr));
  
  return new_process;
}

void *listen_to_other_processes() {
  int sockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buf[MAX_PACKAGE_DATA_LENGTH];

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) printf("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(self.port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
		printf("ERROR on binding\n");
    // exit?
  }
  
  internal_socket_id = sockfd;
  
  message_t *msg = malloc(sizeof(message_t));
  char message_buffer[sizeof(message_t)];
    
    
  while(1) {
    // receive message from other processes
		n = recvfrom(sockfd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0)
			printf("ERROR on recvfrom");
      
    memcpy(msg, message_buffer, sizeof(message_t));
    
    // se recebeu uma mensagem de um servidor backup querendo conectar
    if (msg->type == _MSG_TYPE_CONNECT_PLEASE) {
      
      process_t *new_backup = get_process_from_message(msg, cli_addr);
      new_backup->socket_id = sockfd;
      
      // adiciona o novo processo na lista de outros processos
      list_insert(&other_processes, new_backup);
      
      // responde dizendo OK
      message_t message;
      config_message(&message, _MSG_TYPE_CONNECTED, 0, "", "");
      send_message2(sockfd, message, &(new_backup->address));
      
      printf(BACKUP_CONNECTION_SUCCEDED, new_backup->pid);
    }
    
    // se recebeu uma mensagem pedindo dados de um processo
    else if (msg->type == _MSG_TYPE_PLEASE_GIVE_ME_PROCESSESS_DATA) {
      int has_sent_self = FALSE;
      char buffer[MAX_PACKAGE_DATA_LENGTH];
      char buffer_ip[MAXNAME];
      
      message_t message;
      process_t *p;
      list_t *aux = other_processes;
      
      while(aux != NULL) {
        // para cada processo existente na lista de processos, envia os dados dele para o novo processo
        p = (process_t *) aux->value;
        
        // se não enviou a struct do próprio processo, envia primeiro
        if (!has_sent_self) {
          p = &self;
          has_sent_self = TRUE;
        
        // se já enviou, avança na lista
        } else {
          aux = aux->next;
        }
        memcpy(buffer, p, sizeof(process_t));
        
        // envia os dados
        config_message2(&message, _MSG_TYPE_PROCESS_DATA, 0, buffer, "");
        send_message2(sockfd, message, &cli_addr);
        
        // espera o processo pedir mais um
        n = recvfrom(sockfd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &cli_addr, &clilen);
    		if (n < 0)
    			printf("ERROR on recvfrom");
      }
      config_message(&message, _MSG_TYPE_END_OF_PROCESS_DATA, 0, "", "");
      send_message2(sockfd, message, &cli_addr);
    }
    
    // se for um backup dando oi para outro
    else if (msg->type == _MSG_TYPE_BACKUP_TO_BACKUP_CONNECT_PLEASE) {
      
      process_t *new_backup = get_process_from_message(msg, cli_addr);
      new_backup->socket_id = sockfd;
      
      printf(BACKUP_CONNECTION_SUCCEDED, new_backup->pid);
      
      // adiciona o novo processo na lista de outros processos
      list_insert(&other_processes, new_backup);
      
      print_processes_list();
      
      // responde dizendo OK
      message_t message;
      config_message(&message, _MSG_TYPE_CONNECTED, 0, "", "");
      send_message2(sockfd, message, &(new_backup->address));
    }
    
    // se for uma mensagem de healthcheck
    else if (msg->type == _MSG_TYPE_ARE_YOU_OK) {
      printf("> I'm fine, thanks!\n");
      
      // responde dizendo OK
      message_t message;
      config_message(&message, MSG_TYPE_OK, 0, "", "");
      send_message2(sockfd, message, &cli_addr);
    }
  }
}

void *primary_healthcheck() {
  message_t message;
  process_t *primary;
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  
  // configura mensagem perguntando se está tudo ok
  config_message(&message, _MSG_TYPE_ARE_YOU_OK, 0, "", "");
  
  primary = (process_t *) other_processes->value;
  
  while(1) {
    usleep(HEALTHCHECK_TIMEOUT * 1000000);
    printf("> Are you ok, primary?\n");
    
    // envia mensagem
    send_message2(primary->socket_id, message, &(primary->address));
    
    // aguarda resposta do primário dizendo que está ok
    receive_message2(primary->socket_id, buffer, MAX_PACKAGE_DATA_LENGTH);
  }
}

//
// main
//
int main(int argc, char **argv) {
  if (argc < 2) {
    printf(MISSING_PARAMETER);
    exit(1);
  }
  
  // inicializa struct que representa o própio processo (self)
  init(argc, argv);
  
  printf("Servidor %s rodando.\nPid: %d, Ip: %s, Porta: %d\n\n", self.role == 0 ? "primário" : "backup", self.pid, self.ip, self.port);
  
  // se for um processo backup
  if (self.role == BACKUP) {
    // conecta com o primário
    connect_to_primary(argv[2], &other_processes, &self);
    
    // e recebe do primário os dados dos outros processos backup para se conectar a eles também
    get_other_processes_data(&other_processes, &self);
    
    // conecta aos outros processos backup
    connect_to_others(argv, &other_processes, &self);
    
    // cria uma thread pra ficar testando se o primário ainda está vivo
    pthread_t tid_healthcheck;
    pthread_create(&tid_healthcheck, NULL, primary_healthcheck, NULL);
  }
  
  // cria uma thread pra ficar escutando os outros processos server
  pthread_t tid_listen;
  pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
  pthread_join(tid_listen, NULL);
    
  // tudo pronto, processo pode agir normalmente
  // do_something();
}
