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

void connect_to_primary(char *ip) {
  process_t *primary = malloc(sizeof(process_t));
  
  strcpy(primary->ip, ip);
  primary->role = PRIMARY;
  primary->port = DEFAULT_PORT;
  primary->socket_id = create_socket(primary->ip, DEFAULT_PORT, &(primary->address));
  
  // adiciona o primário na lista dos outros processos
  other_processes = list_make_node(primary);
  
  // serializa a struct process_t do self
  char self_data_buffer[MAX_PACKAGE_DATA_LENGTH];
  memcpy(self_data_buffer, &self, sizeof(self));
  
  // envia uma mensagem pro primário pedindo pra conectar e enviando seus dados
  message_t message;
  config_message(&message, _MSG_TYPE_CONNECT_PLEASE, 0, self_data_buffer, "");
  send_message2(primary->socket_id, message, &(primary->address));
  
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  
  // aguarda resposta do primário dizendo OK
  receive_message2(primary->socket_id, buffer, MAX_PACKAGE_DATA_LENGTH);
  
  message_t *msg = malloc(sizeof(message_t));
  memcpy(msg, buffer, sizeof(message_t));
  
  // se deu algum erro, exit
  if (msg->type != _MSG_TYPE_CONNECTED) {
    printf("ERRO AO CONECTAR COM PRIMÁRIO!\n");
    exit(-1);
  }
  free(msg);
  
  printf("> Conexão com processo primário feita com sucessso!\n\n");
}

void get_other_processes_data() {
  int there_are_processes_remaining = TRUE;
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  message_t *msg = malloc(sizeof(message_t));
  process_t *primary = (process_t *) other_processes->value;
  
  // agora este processo receberá do primário os dados dos outros processos
  while(there_are_processes_remaining) {
    // envia uma mensagem pro primário pedindo dados de um processo
    message_t message;
    config_message(&message, _MSG_TYPE_PLEASE_GIVE_ME_PROCESSESS_DATA, 0, "", "");
    send_message2(primary->socket_id, message, &(primary->address));
    
    // aguarda resposta do primário contendo os dados de um dos outros processos de backup (se houver)
    receive_message2(primary->socket_id, buffer, MAX_PACKAGE_DATA_LENGTH);
    
    // deserializa
    memcpy(msg, buffer, sizeof(message_t));
    
    // acabaram os processos
    if (msg->type == _MSG_TYPE_END_OF_PROCESS_DATA) {
      there_are_processes_remaining = FALSE;
    }
    
    // se veio algo no campo data da mensagem, tem processo
    // se não veio, acabaram os processos da lista do primário e acabou o laço
    if (strcmp(msg->data, "") != 0) {
      process_t *p = malloc(sizeof(process_t));
      memcpy(p, msg->data, sizeof(process_t));
      
      // se forem dados do primário, apenas atualiza o pid dele na lista
      if (p->role == PRIMARY) {
        primary->pid = p->pid;
      } else {
        // se o processo recebido não for o próprio
        if (p->pid != self.pid) {
          list_insert(&other_processes, p);
        } else {
          memcpy(&self.address, &(p->address), sizeof(p->address));
          strcpy(self.ip, p->ip);
        }
      }
    }
  }
  print_processes_list();
}

// void send_message() {
//   // manda uma mensagem de login pro servidor
//   message_t message;
//   config_message(&message, MSG_TYPE_LOGIN, 0, username, "");
//   send_message(socket_id, message);
//
//   // espera a resposta (ack)
//   char ack_buffer[MAX_PACKAGE_DATA_LENGTH];
//   receive_message(socket_id, ack_buffer, MAX_PACKAGE_DATA_LENGTH);
//
//   if (strcmp(ack_buffer, "ok") != 0) {
//     printf("%s", ack_buffer);
//     return ERROR;
//   }
// }

void connect_to_others(char **argv) {
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  list_t *aux = other_processes;
  process_t *p;
  
  // percorre a lista de outros processos
  while (aux != NULL) {
    p = (process_t *) aux->value;
    
    aux = aux->next;
    
    // se for o primário, pula
    if (p->role == PRIMARY) {
      continue;
    }
    
    // pra cada um, abre um socket
    p->socket_id = create_socket(p->ip, p->port, &(p->address));

    char buffer[MAX_PACKAGE_DATA_LENGTH];
    memcpy(buffer, &self, sizeof(process_t));
    
    // e envia uma mensagem dizendo "oi, eu sou o processo tal, vamos nos conectar?"
    message_t message;
    config_message(&message, _MSG_TYPE_BACKUP_TO_BACKUP_CONNECT_PLEASE, 0, buffer, "");
    send_message2(p->socket_id, message, &(p->address));
    
    // aguarda a resposta
    receive_message2(p->socket_id, buffer, MAX_PACKAGE_DATA_LENGTH);
    
    printf("> Conexão com processo backup com pid %d feita com sucessso!\n\n", p->pid);
  }
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
      
      printf("> Conexão com processo backup feita com sucesso!\nPid: %d\n\n", new_backup->pid);
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
      
      printf("Um processo novo quer conectar. Seu pid é %d e o socket_id %d\n", new_backup->pid, new_backup->socket_id);
      
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
    connect_to_primary(argv[2]);
    
    // e recebe do primário os dados dos outros processos backup para se conectar a eles também
    get_other_processes_data();
    
    // conecta aos outros processos backup
    connect_to_others(argv);
    
    // cria uma thread pra ficar testando se o primário ainda está vivo
    pthread_t tid_healthcheck;
    pthread_create(&tid_healthcheck, NULL, primary_healthcheck, NULL);
    pthread_join(tid_healthcheck, NULL);
  }
  
  // cria uma thread pra ficar escutando os outros processos server
  pthread_t tid_listen;
  pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
  pthread_join(tid_listen, NULL);
    
  // tudo pronto, processo pode agir normalmente
  // do_something();
}
