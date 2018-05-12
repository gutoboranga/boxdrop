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

void sync_server() {
}

void receive_file(char *file) {
}

void send_file(char *file) {
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
		
	struct in_addr ip;
	ip.s_addr = (unsigned long) serv_addr.sin_addr.s_addr;
		
	clilen = sizeof(struct sockaddr_in);
	
	while (1) {
    char message_buffer[sizeof(message_t)];
    
		/* receive from socket */
		n = recvfrom(sockfd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0)
			printf("ERROR on recvfrom");
      
    message_t *msg = malloc(sizeof(message_t));
    memcpy(msg, message_buffer, sizeof(message_t));
    
    // se recebeu uma mensagem que vai receber um arquivo
    if (msg->type == MSG_TYPE_SEND_FILE) {
      printf(">> datagram told me client will upload file %s. I'm ready!\n\n", msg->filename);
      
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
    
    
    } else {
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