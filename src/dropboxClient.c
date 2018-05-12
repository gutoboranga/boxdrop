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

  // conecta com o servidor ?
  // int result = connect(socket_id, (struct sockaddr *) &server_address, sizeof(server_address));
  // if (result < 0) {
  //  return ERROR;
  // }
  
  // int max_package = 64;
  // int done_reading = 0;
  // int start_index = 0;
  // char *buffer = malloc(sizeof(char) * max_package);
  //
  // while(!done_reading) {
  //   int chars_read = read_file_content("teste.txt", buffer, start_index, max_package);
  //
  //   start_index += chars_read;
  //
  //   // se leu alguma coisa at all
  //   if (chars_read > 0) {
  //
  //     // a ideia é que aqui vai rolar a transmissão de um pacote contendo o conteúdo do buffer,
  //     // que é um pedacinho do arquivo
  //
  //     // teste
  //     int n = sendto(socket_id, buffer, chars_read, 0, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
  //   	if (n < 0)
  //   		printf("ERROR sendto");
  //
  //     struct sockaddr_in from;
  //   	unsigned int length = sizeof(struct sockaddr_in);
  //
  //   	n = recvfrom(socket_id, buffer, 256, 0, (struct sockaddr *) &from, &length);
  //   	if (n < 0)
  //   		printf("ERROR recvfrom");
  //
  //   	printf("Got an ack: %s\n", buffer);
  //     // fim teste
  //
  //     // printf("------------------------\nRESULT = %i\nBUFFER = %s\nDONE_READING = %i\n", chars_read, buffer, done_reading);
  //   }
  //
  //   // se leu menos do que o máximo que poderia, terminou de ler o arquivo.
  //   if (chars_read < max_package) {
  //     done_reading = 1;
  //   }
  // }
  //
  // // teste
  // printf("------------------------\nAcabou de ler todo o arquivo!\n");
  // close(socket_id);
  // // fim teste
  
  
  // TESTE
  // char buffer[256];
	// printf("Enter the message: ");
	// bzero(buffer, 256);
	// fgets(buffer, 256, stdin);
  //
	// int n = sendto(socket_id, buffer, strlen(buffer), 0, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
	// if (n < 0)
	// 	printf("ERROR sendto");
  //
  //
  // struct sockaddr_in from;
	// unsigned int length = sizeof(struct sockaddr_in);
  //
	// n = recvfrom(socket_id, buffer, 256, 0, (struct sockaddr *) &from, &length);
	// if (n < 0)
	// 	printf("ERROR recvfrom");
  //
	// printf("Got an ack: %s\n", buffer);
  //
	// close(socket_id);
  
  // FIM DO TESTE
  
  return socket_id;
}

void sync_client() {
}

int send_file(char *file) {
  // printf("---- should send %s to %i\n", file, socket_identifier);
  
  int done_reading = 0;
  int start_index = 0;
  
  char *buffer = malloc(sizeof(char) * MAX_PACKAGE_DATA_LENGTH);
  char *ack_buffer = malloc(sizeof(char) * MAX_PACKAGE_DATA_LENGTH);
  char message_buffer[sizeof(message_t)];
  
  struct sockaddr_in from;
  unsigned int length = sizeof(struct sockaddr_in);
  
  file_info_t f;
  strcpy(f.name, file);
  strcpy(f.extension, file);
  strcpy(f.last_modified, "yesterday"); // ARRUMAR
  f.size = 12; // ARRUMAR
  
  message_t message;
  message.type = MSG_TYPE_SEND_FILE;
  message.size = 0;
  message.file = f;
  
  // envia primeira mensagem avisando que vai chegar uns dados
  
  memcpy(message_buffer, &message, sizeof(message));
  
  int n = sendto(socket_identifier, message_buffer, sizeof(message_buffer), 0, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
  if (n < 0) {
    printf("ERROR sendto");
    return ERROR;
  }
  
  n = recvfrom(socket_identifier, ack_buffer, MAX_PACKAGE_DATA_LENGTH, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    printf("ERROR recvfrom");
    return ERROR;
  }
  
  printf("> FIRST ACK: %s\n", ack_buffer);
  
  while(!done_reading) {
    int chars_read = read_file_content(file, buffer, start_index, MAX_PACKAGE_DATA_LENGTH);
    
    printf ("buffer: %.128s\nbuffer size: %lu\n\n", buffer, strlen(buffer));
    
    start_index += chars_read;
    
    // se leu alguma coisa at all
    if (chars_read > 0) {
        
      // a ideia é que aqui vai rolar a transmissão de um pacote contendo o conteúdo do buffer,
      // que é um pedacinho do arquivo
      
      message.type = MSG_TYPE_DATA;
      // strcpy(message.data, buffer);
      // memcpy(message.data, buffer, chars_read);
      stpncpy(message.data, buffer, sizeof(message.data));
      message.size = chars_read;
      
      printf("message.data = %s\n\n", message.data);
      
      memcpy(message_buffer, &message, sizeof(message));
      
      int n = sendto(socket_identifier, buffer, MAX_PACKAGE_DATA_LENGTH, 0, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
      // int n = sendto(socket_identifier, message_buffer, sizeof(message_buffer), 0, (const struct sockaddr *) &server_address, sizeof(struct sockaddr_in));
      if (n < 0) {
        printf("ERROR sendto");
        return ERROR;
      }
      
      n = recvfrom(socket_identifier, ack_buffer, MAX_PACKAGE_DATA_LENGTH, 0, (struct sockaddr *) &from, &length);
      if (n < 0) {
        printf("ERROR recvfrom");
        return ERROR;
      }
      printf("> ACK: %s\n", ack_buffer);
      
      // printf("------------------------\nRESULT = %i\nBUFFER = %s\nDONE_READING = %i\n", chars_read, buffer, done_reading);
    }
    
    // se leu menos do que o máximo que poderia, terminou de ler o arquivo.
    if (chars_read < MAX_PACKAGE_DATA_LENGTH) {
      done_reading = 1;
    }
  }
  
  printf("> Arquivo %s enviado com sucesso!\n", file);
  
  free(buffer);
  free(ack_buffer);
  
  return SUCCESS;
}

void get_file(char *file) {
  // envia uma mensagem pro servidor pedindo o tal arquivo
  
  // aguarda resultado
  
  // se deu erro / nao existe, return ERROR
  
  // senão, em seguida vai receber várias mensagens contendo pedaços do arquivo, dentro
  // de um loop em que recebe msg, escreve no arquivo o pedaço recebido, envia ack
}

void delete_file(char *file) {
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
   
   char *user = argv[1];
   char *adress = argv[2];
   int door = atoi(argv[3]);
   
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
   get_sync_dir(user);
   
   // Início da linha de comando para o usuário:
   // fica lendo os comandos até rolar um exit, daí acaba tudo
   char *command = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   char *argument = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   
   send_file("teste.txt");
   
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
       
       send_file(argument);
       // aqui sim faz o que deve ser feito
     }
     
     else if (strcmp(command, CLIENT_DOWNLOAD_CMD) == 0) {
       // garante que argument não é inválido
       if (strcmp(argument, "") == 0) {
         printf(CLIENT_DOWNLOAD_CMD_MISSING_ARGUMENT);
         continue;
       }
       
       // aqui sim faz o que deve ser feito
     }
     
     else if (strcmp(command, CLIENT_LIST_SERVER_CMD) == 0) {
     }
     
     else if (strcmp(command, CLIENT_LIST_CLIENT_CMD) == 0) {
     }
     
     else if (strcmp(command, CLIENT_GET_SYNC_DYR_CMD) == 0) {
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
