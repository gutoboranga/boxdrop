#include <dropboxClient.h>
#include <dropboxUtil.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *username;
char *adress;
int door;
int socket_identifier;
struct sockaddr_in server_address;


int login_server(char *host, int port) {
	// procura pelo servidor contido em host
  struct hostent *server;
	server = gethostbyname(host);
	if (server == NULL) {
    return ERROR;
  }
	
  // cria o socket
  int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_id == ERROR) {
    return ERROR;
  }
	
  // seta umas paradas do servidor
  struct sockaddr_in s_address;
  
	s_address.sin_family = AF_INET;
	s_address.sin_port = htons(port);
	s_address.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(s_address.sin_zero), 8);
  
  server_address = s_address;

  // manda uma mensagem de login pro servidor
  message_t message;
  config_message(&message, MSG_TYPE_LOGIN, 0, username, "");
  send_message(socket_id, message);
  
  // espera a resposta (ack)
  char ack_buffer[MAX_PACKAGE_DATA_LENGTH];
  receive_message(socket_id, ack_buffer, MAX_PACKAGE_DATA_LENGTH);
  
  return socket_id;
}


void sync_client() {
}


int send_message(int socket_id, message_t message) {
  char message_buffer[sizeof(message_t)];
  
  // "serializa" a struct mensagem (como um buffer) para enviar pelo socket
  memcpy(message_buffer, &message, sizeof(message));
  
  int n = sendto(socket_id, message_buffer, sizeof(message_buffer), 0, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
  if (n < 0) {
    printf("ERROR sendto\n");
    return ERROR;
  }
  return SUCCESS;
}


int receive_message(int socket_id, char *buffer, int size) {
  struct sockaddr_in from;
  unsigned int length = sizeof(struct sockaddr_in);
  
  int n = recvfrom(socket_id, buffer, size, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    printf("ERROR recvfrom\n");
    return ERROR;
  }
  
  return SUCCESS;
}


int send_file(char *file) {
  int done_reading = 0;
  int start_index = 0;
  
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  char ack_buffer[MAX_PACKAGE_DATA_LENGTH];
  
  message_t message;
  
  // se não existe o arquivo a ser enviado, erro
  if (!file_exists(file)) {
    printf(CLIENT_UPLOAD_NO_SUCH_FILE, file);
    return ERROR;
  }
  
  // enquanto houver coisa para ler no arquivo, vai lendo ele aos pedacinhos (de tamanho MAX_PACKAGE_DATA_LENGTH = 128)
  // e os enviando para o servidor
  while(!done_reading) {
    int chars_read = read_file_content(file, buffer, start_index, MAX_PACKAGE_DATA_LENGTH);
    
    start_index += chars_read;
  
    // se leu alguma coisa at all
    if (chars_read > 0) {
      // envia mensagem com um pedaço do arquivo
      config_message(&message, MSG_TYPE_DATA, chars_read, (char *) &buffer, file);
      send_message(socket_identifier, message);
      
      // espera um ack dizendo ok
      receive_message(socket_identifier, ack_buffer, MAX_PACKAGE_DATA_LENGTH);
    }
  
    // se leu menos do que o máximo que poderia, terminou de ler o arquivo.
    if (chars_read < MAX_PACKAGE_DATA_LENGTH) {
      done_reading = 1;
    }
  }
  
  return SUCCESS;
}


int get_file(char *file) {
  char message_buffer[sizeof(message_t)];
  char ack_buffer[sizeof(message_t)];
  
  message_t message;
  
  // envia mensagem pedindo arquivo para o servido
  config_message(&message, MSG_TYPE_GET_FILE, 0, "", file);
  send_message(socket_identifier, message);
  
  // espera a resposta de que o servidor recebeu
  receive_message(socket_identifier, ack_buffer, MAX_PACKAGE_DATA_LENGTH);
  
  // se a resposta não for "ok", é uma mensagem de erro. printa ela e retorna erro
  if (strcmp(ack_buffer, "ok") != 0) {
    printf("%s", ack_buffer);
    return ERROR;
  }
  
  // cria uma string c o path absoluto do arquivo de destino
  char *full_path = malloc(sizeof(char) * PATH_MAX_SIZE);
  full_path = build_user_dir_path(username);
  strcat(full_path, "/");
  strcat(full_path, file);
  
  message_t *msg = malloc(sizeof(message_t));
  int received_all = 0;
  
  // antes de receber os dados do arquivo, envia uma mensagem pra manter dualidade envia/recebe
  config_message(&message, MSG_TYPE_OK, 0, "", "");
  send_message(socket_identifier, message);
  
  // enquanto houver coisa para receber
  while (!received_all) {
  
    // espera receber um pedaço de arquivo
    receive_message(socket_identifier, ack_buffer, sizeof(message_t));
    
    // coloca na variável msg do tipo message_t. Em outras palavras, deserializa a mesnagem recebida
    memcpy(msg, ack_buffer, sizeof(message_t));
    
    // se chegar uma mensagem de fim de transmissão quer dizer que deve sair do loop
    if (msg->type == MSG_END_OF_TRANSMISSION) {
      received_all = TRUE;
      continue;
    }
    
    // pega apenas a parte importante da área de dados da mensagem
    char data_on_right_size[MAX_PACKAGE_DATA_LENGTH + 1];
    memcpy(data_on_right_size, msg->data, msg->size);
    data_on_right_size[msg->size] = '\0';
    
    // escreve no arquivo
    write_to_file(full_path, data_on_right_size);
    
    // envia a mensagem de ok pro servidor
    config_message(&message, MSG_TYPE_OK, 0, "", "");
    send_message(socket_identifier, message);
  }
  
  free(full_path);
  free(msg);
  
  return SUCCESS;
}


void delete_file(char *file) {
}


void list_server() {
  // manda mensagem pedindo lista de arquivos
  message_t message;
  config_message(&message, MSG_TYPE_LIST_SERVER, 0, "", "");
  send_message(socket_identifier, message);
  
  // espera resposta
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  struct sockaddr_in from;
  unsigned int length = sizeof(struct sockaddr_in);
  
  // receive_message(socket_identifier, response_buffer, sizeof(message_t));
  int n = recvfrom(socket_identifier, buffer, MAX_PACKAGE_DATA_LENGTH, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    printf("ERROR recvfrom\n");
  }
  
  // deserializa a mensagem recebida
  message_t *msg = malloc(sizeof(message_t));
  memcpy(msg, buffer, sizeof(message_t));
  
  printf("%s", msg->data);
}

void list_client() {
  char *buffer = malloc(sizeof(char) * MAX_PACKAGE_DATA_LENGTH);
  ls(build_user_dir_path(username), buffer);
  
  printf("%s", buffer);
  
  free(buffer);
}

void close_session() {
  close(socket_identifier);
  // exit(0);
}


void get_sync_dir(char *user) {
  // cria uma string com o path completo "/home/sync_dir_<USER NAME>"
  char *path = build_user_dir_path(user);
  
  // se não existe o dir ainda, cria
  if(!dir_exists(path)) {
    mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
  }
  
  // let it go
  free(path);
}


int main(int argc, char *argv[]) {
  if(argc != CLIENT_PARAMS_NUMBER) {
      printf(CLIENT_INVALID_PARAMS_NUMBER);
      return ERROR;
   }
   
   username = argv[1];
   adress = argv[2];
   door = atoi(argv[3]);
   
   // Tenta conectar com o servidor
   int id = login_server(adress, door);
   if (id == ERROR) {
     printf(CLIENT_ERROR_LOGGING_IN, adress, door);
     return ERROR;
   }
   
   // se deu certo, salva o socket_id em uma var global
   socket_identifier = id;
   
   // Chamada para ver se o diretorio do usuario existe na máquina local
   // do client (se não, o cria) e sincronizar com o servidor
   get_sync_dir(username);
   
   // Início da linha de comando para o usuário:
   // fica lendo os comandos até rolar um exit, daí acaba tudo
   char *command = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   char *argument = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   
   while(1) {
     printf("> ");
     
     read_command(command, argument, CLIENT_COMMAND_MAX_SIZE);
     
     // confere cada possível comando:
     if (strcmp(command, CLIENT_EXIT_CMD) == 0) {
       // faz logout do sistema e retorna
       return SUCCESS;
     }
     
     else if (strcmp(command, CLIENT_UPLOAD_CMD) == 0) {
       // garante que argument não é inválido
       if (strcmp(argument, "") == 0) {
         printf(CLIENT_UPLOAD_CMD_MISSING_ARGUMENT);
         continue;
       }
       
       if (send_file(argument) == SUCCESS) {
         printf(CLIENT_UPLOAD_SUCCESS, argument);
       }
       
       // aqui sim faz o que deve ser feito
     }
     
     else if (strcmp(command, CLIENT_DOWNLOAD_CMD) == 0) {
       // garante que argument não é inválido
       if (strcmp(argument, "") == 0) {
         printf(CLIENT_DOWNLOAD_CMD_MISSING_ARGUMENT);
         continue;
       }
       
       if (get_file(argument) == SUCCESS) {
         printf(CLIENT_DOWNLOAD_SUCCESS, argument);
       }
       // aqui sim faz o que deve ser feito
     }
     
     else if (strcmp(command, CLIENT_LIST_SERVER_CMD) == 0) {
       list_server();
     }
     
     else if (strcmp(command, CLIENT_LIST_CLIENT_CMD) == 0) {
       list_client();
     }
     
     else if (strcmp(command, CLIENT_GET_SYNC_DYR_CMD) == 0) {
       get_sync_dir(username);
     }
     
     else {
       printf(CLIENT_INVALID_COMMAND);
       printf(CLIENT_COMMANDS_HELP);
     }
   }
   
   free(command);
   free(argument);
   
   return SUCCESS;
}
