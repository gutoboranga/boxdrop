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
#include <process.h>
#include <dropboxServer.h>

process_t self;
extern list_t *other_processes;

int internal_port = DEFAULT_PORT;
int internal_socket_id;

pthread_t tid_listen;
pthread_t tid_healthcheck;
pthread_t tid_client;

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
    
    printf("pid: %d\trole: %s\tip: %s\tport: %d\n", p->pid, p->role == 0 ? "primário" : "backup", p->ip, p->port);
    
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
      new_backup->socket_id_2 = create_socket(new_backup->ip, new_backup->port - PORT_OFFSET, &(new_backup->address_2));
      
      // adiciona o novo processo na lista de outros processos
      list_insert(&other_processes, new_backup);
      
      // responde dizendo OK
      message_t message;
      config_message(&message, _MSG_TYPE_CONNECTED, 0, "", "");
      send_message2(sockfd, message, &(new_backup->address));
      
      // TODO: abrir socket na porta do processo - 1000 pra poder depois passar adiante as mensagens recebidas do client
      
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
      // printf("> I'm fine, thanks!\n");
      
      // responde dizendo OK
      message_t message;
      config_message(&message, MSG_TYPE_OK, 0, "", "");
      send_message2(sockfd, message, &cli_addr);
    }
    
    // se receber uma mensagem de outro processo avisando que o líder atual falhou
    else if (msg->type == _MSG_TYPE_LEADER_HAS_FAILED) {
      printf(BACKUP_HAS_BEEN_WARNED_OF_PRIMARY_FAILURE);
      pthread_cancel(tid_healthcheck);
    }
    
    // se receber uma mensagem de outro processo p iniciando uma eleição é porque
    // o processo corrente tem pid maior que p.
    //
    // neste caso, o corrente vai enviar uma mensagem de eleição para os processos
    // com pid maior que o seu e segue o baile
    else if (msg->type == _MSG_TYPE_ELECTION) {
      printf(BACKUP_RECEIVED_ELECTION_MESSAGE);
      create_election();
    }
    
    // se receber uma mensagem de outro processo dizendo que ele é o novo líder
    else if (msg->type == _MSG_TYPE_I_AM_THE_LEADER) {
      // cria um processo temporário com os dados atualizados que vieram na mensagem
      process_t *new_leader = get_process_from_message(msg, cli_addr);
      
      // printf("O processo com pid %d é o novo líder.\n", new_leader->pid);
      
      // pega o processo com mesmo pid na lista other_processes
      process_t *new_leader_in_list = get_process_from_pid(&other_processes, new_leader->pid);
      
      // remove o primário antigo da lista
      remove_primary();
      
      // atualiza os dados do novo primário
      new_leader_in_list->role = new_leader->role;
      new_leader_in_list->port = new_leader->port;
      
      new_leader_in_list->socket_id = create_socket(new_leader_in_list->ip, new_leader_in_list->port, &(new_leader_in_list->address));
      
      printf(NEW_PRIMARY_CONNECTION_SUCCEDED, new_leader_in_list->pid);
      print_processes_list();
      
      // cria novamente a thread que fica testando se o primário ainda está vivo
      pthread_create(&tid_healthcheck, NULL, primary_healthcheck, NULL);
      
      // libera o temporário
      free(new_leader);
    }
  }
}

void *primary_healthcheck() {
  message_t message;
  process_t *primary;
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  
  // configura mensagem perguntando se está tudo ok
  config_message(&message, _MSG_TYPE_ARE_YOU_OK, 0, "", "");
  
  primary = get_primary(other_processes);
  
  while(1) {
    usleep(HEALTHCHECK_FREQUENCY * 1000000);
    // printf("> Are you ok, primary?\n");
    
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
      printf(BACKUP_HAS_DETECTED_PRIMARY_FAILURE);
      // avisa o pessoal que o primário falhou
      warn_leader_failure();
      
      // inicia uma eleição
      create_election();
      
      // printf("Se chegou aqui acho que a eleição já foi resolvida...\n");
      
      // cancela a thread atual de healthcheck
      pthread_cancel(tid_healthcheck);
    }
  }
}

process_t *get_primary(list_t *head) {
  list_t *aux = head;
  
  while (aux != NULL) {
    if ((int) ((process_t *) aux->value)->role == PRIMARY) {
      return aux->value;
    }
    aux = aux->next;
  }
  
  return NULL;
}

// void handle_primary_failure() {
//   // printf("Vou cancelar a thread de healthcheck e remover o primário!\n");
//   // remove o primário da lista e avisa todo mundo que ele falhou
//   remove_primary();
//
//   // pára a thread de healthcheck do primário
//   pthread_cancel(tid_healthcheck);
// }

void remove_primary() {
	// remove o processo primário da lista
	process_t *primary = get_primary(other_processes);
  if (primary != NULL) {
    other_processes = list_remove_with_pid(other_processes, primary->pid);
  }
}

void warn_leader_failure() {
  // enviar mensagem pra todos dizendo para parar a thread de healthcheck pois já detectou falha!
	message_t message;
  config_message(&message, _MSG_TYPE_LEADER_HAS_FAILED, 0, "", "");
  int count = broadcast_message(&message, &other_processes, 0);
  
  printf(BACKUP_HAS_WARNED_ABOUT_PRIMARY_FAILURE, count);
  
  // se não enviou pra ninguém, provavelmente é o único processo vivo ainda!
  // neste caso, ele é o novo líder
  if (count == 0) {
    become_leader();
    // printf("Ops, parece que tô sozinho nessa! Vou virar líder então!\n");
  }
}

void create_election() {
  // envia msg pra todos COM PID MAIOR QUE O SEU dizendo que houve uma falha no primário
  message_t message;
  config_message(&message, _MSG_TYPE_ELECTION, 0, "", "");
  int count = broadcast_message(&message, &other_processes, self.pid);
  
  // se não houver nenhum com pid maior que o seu, este processo é o novo líder!
  if (count == 0) {
    printf(BACKUP_WITH_BIGGEST_PID_WILL_BECOME_LEADER);
    become_leader();
  }
}

void become_leader() {
  // remove o antigo primário da sua lista
  remove_primary();
  print_processes_list();
  
  // atualiza seus valores
  self.role = PRIMARY;
  self.port = DEFAULT_PORT;
  
  printf(WATER_DIVIDER);
  printf(SERVER_UP_AND_RUNNING);
  
  char self_data_buffer[MAX_PACKAGE_DATA_LENGTH];
  memcpy(self_data_buffer, &self, sizeof(process_t));
    
  // envia pra todos
  message_t message;
  config_message(&message, _MSG_TYPE_I_AM_THE_LEADER, 0, self_data_buffer, "");
  int count = broadcast_message(&message, &other_processes, 0);
  
  // cancela a thread de comunicação com os outros processos
  // mas não se preocupe, ela será reiniciada quando a execução voltar à thread main
  pthread_cancel(tid_listen);
  
  // reabre os sockets secundários para repassar as mensagens do cliente
  open_secondary_sockets(&other_processes, &self);
  
  // printf("Cancelei a thread antiga de listen\n");
  
  //
  // // reinicia a thread pra ficar escutando os outros processos
  // pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
  // pthread_join(tid_listen, NULL);
  //
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
  
  printf(SERVER_UP_AND_RUNNING);

  // se for um processo backup
  if (self.role == BACKUP) {
    
    // conecta com o primário
    connect_to_primary(argv[2], &other_processes, &self);

    // e recebe do primário os dados dos outros processos backup para se conectar a eles também
    get_other_processes_data(&other_processes, &self);
    
    // conecta aos outros processos backup
    connect_to_others(argv, &other_processes, &self);
    
    // cria uma thread pra ficar testando se o primário ainda está vivo
    pthread_create(&tid_healthcheck, NULL, primary_healthcheck, NULL);
  }
  
  // se a thread for cancelada, será reerguida por motivos de rebinding caso um backup vire um líder
  while(1) {
    // cria uma thread pra ficar escutando os outros processos server
    pthread_create(&tid_listen, NULL, listen_to_other_processes, NULL);
    
    // cria uma thread pra responder aos requests do cliente (parte 1 do trabalho)
    pthread_create(&tid_client, NULL, handle_client_requests, (void *) &self);
    
    pthread_join(tid_listen, NULL);
  }
  
  // tudo pronto, processo pode agir normalmente
  // do_something();
}
