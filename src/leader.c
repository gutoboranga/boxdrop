#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

int internal_port = 5000;
int internal_socket_id;

void init(int argc, char **argv) {
  // handle process role input
  if (strcmp(argv[1], "primary") == 0) {
    self.role = PRIMARY;
  } else if (strcmp(argv[1], "backup") == 0) {
    self.role = BACKUP;
  } else {
    printf(INVALID_PARAMETER);
    exit(1);
  }
  
  self.port = internal_port;
  self.pid = getpid();
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
  primary->port = internal_port;
  primary->socket_id = create_socket(primary->ip, internal_port, &(primary->address));
  
  printf("PORTA %d\n", internal_port);
  
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
  
  printf("> Conexão com processo primário feita com sucessso!\n");
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
    printf("tem um processo aqui\n");
    
    // se for o primário, pula
    if (p->role == PRIMARY) {
      continue;
    }
    
    printf("TO NO PID %d\n", p->pid);
    
    // pra cada um, abre um socket
    printf("IP DELE: %s\n", p->ip);
    p->socket_id = create_socket(p->ip, internal_port + p->role, &(p->address));
    
    printf("CRIEI SOCKET PRO PID %d\n", p->pid);
    
    // e envia uma mensagem dizendo "oi, eu sou o processo tal, vamos nos conectar?"
    message_t message;
    // config_message(&message, _MSG_TYPE_BACKUP_TO_BACKUP_CONNECT_PLEASE, 0, serialize_process(&self), "");
    config_message(&message, _MSG_TYPE_BACKUP_TO_BACKUP_CONNECT_PLEASE, 0, "ai ai ai ta chegando a hora", "");
    send_message2(p->socket_id, message, &(p->address));
    
    printf("ENVIEI MSG PRO PID %d\n", p->pid);
    
    // aguarda a resposta
    receive_message2(p->socket_id, buffer, MAX_PACKAGE_DATA_LENGTH);
    
    printf("RECEBI MSG DO PID %d\n", p->pid);
    
    printf("> Conexão com processo backup com pid %d feita com sucessso!\n", p->pid);
  }
  
  printf("> conectou c todos outros\n");
}

void print_processes_list() {
  list_t *aux = other_processes;
  printf("-- Outros processos --\n");
  if (aux == NULL) {
    printf("AUX NULL\n");
  }
  while (aux != NULL) {
    process_t *p = (process_t *) aux->value;
    
    printf("pid: %d\trole: %s\n", p->pid, p->role == 0 ? "primário" : "backup");
    
    aux = aux->next;
  }
  printf("----------------------\n");
}


void *listen_to_other_processes() {
  int sockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buf[MAX_PACKAGE_DATA_LENGTH];

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) printf("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
  int x = internal_port + self.role;
  printf("estarei ouvindo em %d\n", x);
	serv_addr.sin_port = htons(x);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
		printf("ERROR on binding\n");
    // exit?
  }
  
  internal_socket_id = sockfd;
  
  message_t *msg = malloc(sizeof(message_t));
  char message_buffer[sizeof(message_t)];
    
  printf("> Aguardando novos processos conectarem ...\n");
    
  while(1) {
    // receive message from other processes
		n = recvfrom(sockfd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0)
			printf("ERROR on recvfrom");
      
    memcpy(msg, message_buffer, sizeof(message_t));
    
    // se recebeu uma mensagem de um servidor backup querendo conectar
    if (msg->type == _MSG_TYPE_CONNECT_PLEASE) {
      
      // de-serializa dados do novo processo, que vieram junto na mensagem
      process_t *new_backup = malloc(sizeof(process_t));
      memcpy(new_backup, msg->data, sizeof(process_t));
      
      // salva o endereço do novo processo
      char *ip_string = inet_ntoa(new_backup->address.sin_addr);
      memcpy(&new_backup->address, &cli_addr, sizeof(cli_addr));
      strcpy(new_backup->ip, inet_ntoa(new_backup->address.sin_addr));
      
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
    
    // se for entre backups
    else if (msg->type == _MSG_TYPE_BACKUP_TO_BACKUP_CONNECT_PLEASE) {
      printf("RECEBI ALGO AI MEU DEUS\n");
      printf("%s\n", msg->data);
    }
  }
}

// void do_something() {
//   while(1) {
//     // 1/2 segundo
//     usleep(500000);
//     printf("did something\n");
//   }
// }

int main(int argc, char **argv) {
  if (argc < 2) {
    printf(MISSING_PARAMETER);
    exit(1);
  }
  
  // inicializa struct que representa o própio processo (self)
  init(argc, argv);
  
  // se for um processo backup
  if (self.role == BACKUP) {
    // checa se foi fornecido o ip do processo primário correspondente
    if (argc < 4) {
      printf(MISSING_PARAMETER_FOR_BACKUP_PROCESS);
      exit(1);
    }
    internal_port = atoi(argv[3]);
    // printf("internal port: %d\n", internal_port);
    
    // se foi, conecta com ele
    connect_to_primary(argv[2]);
    
    // e recebe do primário os dados dos outros processos backup para se conectar a eles também
    get_other_processes_data();
    printf("will conect to others\n");
    // conecta aos outros processos backup
    connect_to_others(argv);
    
    // cria uma thread pra ficar testando se o primário ainda está vivo
    // pthread_t tid_healthcheck;
    // pthread_create(&tid_healthcheck, NULL, primary_healthcheck, NULL);
  }
  
  // cria uma thread pra ficar escutando os outros processos server
  pthread_t tid_listen;
  pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
  pthread_join(tid_listen, NULL);
  
  printf("Processo com id: %d é %s\n", self.pid, self.role == 0 ? "primário" : "backup");
  
  // tudo pronto, processo pode agir normalmente
  // do_something();
}
