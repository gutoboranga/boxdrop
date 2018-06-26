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

pthread_t tid_listen;
pthread_t tid_healthcheck;

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

	printf("ip string: %s\n", ip_string);
  
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
      printf("respondi ok no ip %s e porta %d\n", new_backup->ip, new_backup->port);
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
			printf("recebi uma msg de um processo\n");
      
      process_t *new_backup = get_process_from_message(msg, cli_addr);
      new_backup->socket_id = sockfd;
      
      
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
    usleep(HEALTHCHECK_FREQUENCY * 1000000);
    printf("> Are you ok, primary?\n");
    
    // envia mensagem
    send_message2(primary->socket_id, message, &(primary->address));
    
    // configura timeout no socket
    struct timeval tv;
    tv.tv_sec = HEALTHCHECK_TIMEOUT;
    tv.tv_usec = 0;
    
    if (setsockopt(primary->socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    
    struct sockaddr_in from;
    unsigned int length = sizeof(struct sockaddr_in);
    
    // se recv falhar devido ao timeout
    if(recvfrom(primary->socket_id, buffer, MAX_PACKAGE_DATA_LENGTH, 0, (struct sockaddr *) &from, &length) < 0) {
        printf("timeout!\n");
        
        remove_primary();
//        create_election();
        
    }
  }
}

list_t *get_primary(list_t *head) {
  list_t *aux = head;
  
  while (aux != NULL) {
    if ((int) ((process_t *) aux->value)->role == PRIMARY) {
      return aux;
    }
    aux = aux->next;
  }
  
  return NULL;
}

void remove_primary() {
	// remove o processo primário da lista
//	process_t *primary = get_primary(other_processes);
//  if (primary != NULL) {
//		other_processes = list_remove_with_pid(other_processes, primary->pid);
//  }
}

void create_election() {
  // enviar mensagem pra todos dizendo para parar a thread de healthcheck pois já detectou falha!
	message_t message;
	list_t *aux;
	process_t *p;

  config_message(&message, _MSG_TYPE_LEADER_HAS_FAILED, 0, "", "");

	aux = other_processes;

	while (aux != NULL) {
		p = (process_t *) aux->value;

		send_message2(p->socket_id, message, &(p->address));
		aux = aux->next;
	}
      
  
  // envia msg pra todos COM PID MAIOR QUE O SEU dizendo que houve uma falha no primário
  
  // se não houver nenhum com pid maior que o seu, este processo é o novo líder.
  // neste caso,
  // become_leader();
}

void become_leader() {
  // cancela ambas threads: a que checa o primário e de comunicação com os outros processos
  pthread_cancel(tid_healthcheck);
  pthread_cancel(tid_listen);
  
  // atualiza a porta
  self.port = DEFAULT_PORT;
  
  // reinicia a thread pra ficar escutando os outros processos
  pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
  pthread_join(tid_listen, NULL);
  
  // manda mensagem pra todos processos dizendo que é o novo líder
  
  // outros processos devem atualizar na lista que este é PRIMARY e sua nova porta é 5000
  
  // processos devem reconectar com primario?
  // talvez:
  
  // fecha conexão com o socket antigo
  // socket.close(new_primary->socket_id);
  
  // atualiza os dados e abre o novo socket
  // new_primary->role = PRIMARY;
  // new_primary->port = DEFAULT_PORT;
  // new_primary->socket_id = create_socket(new_primary->ip, DEFAULT_PORT, &(new_primary->address));
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
  
	printf("role: %d\n", self.role);

  // se for um processo backup
  if (self.role == BACKUP) {
		printf("PUTA QUE ME PARIU\n");
    // conecta com o primário
    connect_to_primary(argv[2], &other_processes, &self);
    
	  printf("vou chamar essa funcao desgraçada\n");

    // e recebe do primário os dados dos outros processos backup para se conectar a eles também
    get_other_processes_data(&other_processes, &self);
    
    // conecta aos outros processos backup
    connect_to_others(argv, &other_processes, &self);
    
    // cria uma thread pra ficar testando se o primário ainda está vivo
    pthread_create(&tid_healthcheck, NULL, primary_healthcheck, NULL);
  }
  
  // cria uma thread pra ficar escutando os outros processos server
  pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
  pthread_join(tid_listen, NULL);
    
  // tudo pronto, processo pode agir normalmente
  // do_something();
}
