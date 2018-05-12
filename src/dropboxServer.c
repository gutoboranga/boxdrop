#include <dropboxServer.h>
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

int socket_fd;
socklen_t client_length;
struct sockaddr_in client_address;

void sync_server() {
}

void receive_file(char *file) {
}

int send_file(char *file) {
  int done_reading = 0;
  int start_index = 0;
  
  char buffer[MAX_PACKAGE_DATA_LENGTH];
  char ack_buffer[MAX_PACKAGE_DATA_LENGTH];
  char message_buffer[sizeof(message_t)];
  message_t message;
  
  int n = recvfrom(socket_fd, ack_buffer, MAX_PACKAGE_DATA_LENGTH, 0, (struct sockaddr *) &client_address, &client_length);
  
  // enquanto houver coisa para ler no arquivo, vai lendo ele aos pedacinhos (de tamanho MAX_PACKAGE_DATA_LENGTH = 128)
  // e os enviando para o servidor
  while(!done_reading) {
    int chars_read = read_file_content(file, buffer, start_index, MAX_PACKAGE_DATA_LENGTH);
    printf("READ:\n%.128s\n-------------\n", buffer);
    
    start_index += chars_read;
  
    // se leu alguma coisa at all
    if (chars_read > 0) {
      config_message(&message, MSG_TYPE_DATA, chars_read, (char *) &buffer, file);
      
      memcpy(message_buffer, &message, sizeof(message));
  
      n = sendto(socket_fd, message_buffer, sizeof(message_buffer), 0, (struct sockaddr *) &client_address, sizeof(struct sockaddr));
      if (n < 0) {
        printf("ERROR sendto");
        return ERROR;
      }
      
      n = recvfrom(socket_fd, ack_buffer, MAX_PACKAGE_DATA_LENGTH, 0, (struct sockaddr *) &client_address, &client_length);
      if (n < 0) {
        printf("ERROR recvfrom");
        return ERROR;
      }
    }
  
    // se leu menos do que o máximo que poderia, terminou de ler o arquivo.
    if (chars_read < MAX_PACKAGE_DATA_LENGTH) {
      done_reading = 1;
    }
  }
  
  // quando acabar, envia uma última mensagem ao cliente avisando que acabou de mandar todo o arquivo
  config_message(&message, MSG_END_OF_TRANSMISSION, 0, "", file);
  memcpy(message_buffer, &message, sizeof(message));
  
  n = sendto(socket_fd, message_buffer, sizeof(message_buffer), 0, (const struct sockaddr *) &client_address, sizeof(struct sockaddr_in));
  if (n < 0) {
    printf("ERROR sendto");
    return ERROR;
  }
  
  return SUCCESS;
}

int main(int argc, char *argv[]) {
	int sockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buf[MAX_PACKAGE_DATA_LENGTH];
		
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) printf("ERROR opening socket");
	

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(4000);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);
	 
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
		printf("ERROR on binding");
    
  socket_fd = sockfd;
		
	struct in_addr ip;
	ip.s_addr = (unsigned long) serv_addr.sin_addr.s_addr;
		
	clilen = sizeof(struct sockaddr_in);
  message_t *msg = malloc(sizeof(message_t));
	
  client_address = cli_addr;
  client_length = clilen;
  
  char message_buffer[sizeof(message_t)];
  
	while (1) {
    
		/* receive from socket */
		n = recvfrom(sockfd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0)
			printf("ERROR on recvfrom");
      
    memcpy(msg, message_buffer, sizeof(message_t));
    
    // se recebeu uma mensagem que vai receber um arquivo
    if (msg->type == MSG_TYPE_SEND_FILE) {
      n = sendto(sockfd, "ok, send file!\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
  		if (n  < 0)
  			printf("ERROR on sendto");
    }
    
    // se recebeu dados
    else if (msg->type == MSG_TYPE_DATA) {
      char data_on_right_size[MAX_PACKAGE_DATA_LENGTH + 1];
      memcpy(data_on_right_size, msg->data, msg->size);
      data_on_right_size[msg->size] = '\0';
      
      printf(">> datagram with data:\n%s\nfilename: %s\n", data_on_right_size, msg->filename);
      
      write_to_file(msg->filename, data_on_right_size);
      
      n = sendto(sockfd, "got data!\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
  		if (n  < 0)
  			printf("ERROR on sendto");
    }
    
    // se recebeu requisição para enviar algum arquivo
    else if (msg->type == MSG_TYPE_GET_FILE) {
      if (!file_exists(msg->filename)) {
        n = sendto(sockfd, SERVER_FILE_NOT_FOUND, MAX_PACKAGE_DATA_LENGTH, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
      } else {
        n = sendto(sockfd, "ok", MAX_PACKAGE_DATA_LENGTH, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
        send_file(msg->filename);
      }
    }
    
    // senão...
    else {
      printf("Received a datagram: %s\n\n", message_buffer);
      n = sendto(sockfd, "Got some data\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
  		if (n  < 0)
  			printf("ERROR on sendto");
    }
    
    // printf("Received:\n\ttype: %i\n\tsize: %i\n\tfilename: %s\n", msg->type, msg->size, msg->filename);
    
  }
      
		
		
		/* send to socket */
		
	
	close(sockfd);
	return 0;
}