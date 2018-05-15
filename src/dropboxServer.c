#include <dropboxServer.h>
#include <dropboxUtil.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int socket_fd;
socklen_t client_length;
struct sockaddr_in client_address;
char *username;
char *user_dir_path;
int num_connections = 0;

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


int send_all() {
  DIR *dir;
  int success = 0;
  struct dirent *ent;

  char *dirpath;
  dirpath = user_dir_path;

  dir = opendir(dirpath);

  if (dir == NULL) {
    return ERROR;
  }

  int n;
  message_t message;
  char message_buffer[sizeof(message_t)];

  // itera sobre os arquivos
  while ((ent = readdir(dir)) != NULL) {
    // para todas as entradas exceto . e ..
    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
      char filepath[PATH_MAX_SIZE];
      strcpy(filepath, dirpath);
      strcat(filepath, ent->d_name);

      printf("FILE: %s\n", ent->d_name);

      // envia o nome do arquivo de volta para o cliente
      config_message(&message, MSG_TYPE_DATA, MAX_PACKAGE_DATA_LENGTH, ent->d_name, ent->d_name);
      memcpy(message_buffer, &message, sizeof(message));

      n = sendto(socket_fd, message_buffer, sizeof(message_buffer), 0,(struct sockaddr *) &client_address, sizeof(struct sockaddr));
      if (n < 0)
  			printf("ERROR on sendto");

  		n = recvfrom(socket_fd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &client_address, &client_length);
  		if (n < 0)
  			printf("ERROR on recvfrom");

      // if (send_file(filepath) != SUCCESS) {
      //   success = -1;
      // }
    }
  }
  closedir (dir);

  return success;
}



int main(int argc, char *argv[]) {
	int sockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buf[MAX_PACKAGE_DATA_LENGTH];

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) printf("ERROR opening socket");

  // inicializa username e user_dir_path
  username = malloc(sizeof(char) * PATH_MAX_SIZE);
  user_dir_path = malloc(sizeof(char) * PATH_MAX_SIZE);

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

    // se recebeu uma mensagem de um client querendo fazer login
    if (msg->type == MSG_TYPE_LOGIN) {
      if (num_connections >= MAX_CONNECTIONS) {
        printf("Max connections reached\n");
        n = sendto(sockfd, "Max connections reached", MAX_PACKAGE_DATA_LENGTH, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));

        if (n  < 0)
    			printf("ERROR on sendto");

      }
      else {
        printf("%s LOGGED IN\n", msg->data);

        n = sendto(sockfd, "ok", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
    		if (n  < 0)
    			printf("ERROR on sendto");

        // salva o nome do usuario
        strcpy(username, msg->data);

        // cria uma string com o path completo "sync_dir_<USER NAME>/"
        strcpy(user_dir_path, "sync_dir_");
        strcat(user_dir_path, username);
        strcat(user_dir_path, "/");

        printf("USER_DIR_PATH: %s\n", user_dir_path);

        // se não existe o dir ainda, cria
        if(!dir_exists(user_dir_path)) {
          mkdir(user_dir_path, S_IRWXU | S_IRWXG | S_IRWXO);
        }

        num_connections++;
      }
    }

    // se recebeu uma mensagem que vai receber um arquivo
    else if (msg->type == MSG_TYPE_SEND_FILE) {
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

      char *file;
      file = msg->filename;
      char *tld = strrchr(msg->filename, '/');
      if (tld != NULL) {
        tld += 1;
        file = tld;
      }
      printf("file: %s\n", file);

      // cria um path juntando o "sync_dir_<USER NAME>/" com o nome do arquivo
      char *filepath = malloc(sizeof(char) * PATH_MAX_SIZE);
      strcpy(filepath, user_dir_path);
      strcat(filepath, file);

      // ver se tem / no filepath. se tiver, pegar apenas ultima parte, que é o nome do arquivo mesmo
      printf("will write to: %s\n", filepath);

      // escreve no arquivprintf("%s LOGGED IN\n", msg->data);

      n = sendto(sockfd, "ok", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
  		if (n  < 0)
  			printf("ERROR on sendto");

      // salva o nome do usuario
      strcpy(username, msg->data);

      // cria uma string com o path completo "sync_dir_<USER NAME>/"
      strcpy(user_dir_path, "sync_dir_");
      strcat(user_dir_path, username);
      strcat(user_dir_path, "/");

      printf("USER_DIR_PATH: %s\n", user_dir_path);

      // se não existe o dir ainda, cria
      if(!dir_exists(user_dir_path)) {
        mkdir(user_dir_path, S_IRWXU | S_IRWXG | S_IRWXO);
      }
    }

    // se recebeu uma mensagem que vai receber um arquivo
    else if (msg->type == MSG_TYPE_SEND_FILE) {
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

      char *file;
      file = msg->filename;
      char *tld = strrchr(msg->filename, '/');
      if (tld != NULL) {
        tld += 1;
        file = tld;
      }
      printf("file: %s\n", file);

      // cria um path juntando o "sync_dir_<USER NAME>/" com o nome do arquivo
      char *filepath = malloc(sizeof(char) * PATH_MAX_SIZE);
      strcpy(filepath, user_dir_path);
      strcat(filepath, file);

      // ver se tem / no filepath. se tiver, pegar apenas ultima parte, que é o nome do arquivo mesmo
      printf("will write to: %s\n", filepath);

      // escreve no arquivo
      write_to_file(filepath, data_on_right_size);

      free(filepath);

      n = sendto(sockfd, "got data!\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
  		if (n  < 0)
  			printf("ERROR on sendto");
    }

    // se recebeu requisição para enviar algum arquivo
    else if (msg->type == MSG_TYPE_GET_FILE) {
      // cria um path juntando o "sync_dir_<USER NAME>/" com o nome do arquivo
      char *filepath = malloc(sizeof(char) * PATH_MAX_SIZE);
      strcpy(filepath, user_dir_path);
      strcat(filepath, msg->filename);

      if (!file_exists(filepath)) {
        n = sendto(sockfd, SERVER_FILE_NOT_FOUND, MAX_PACKAGE_DATA_LENGTH, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
      } else {
        n = sendto(sockfd, "ok", MAX_PACKAGE_DATA_LENGTH, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
        send_file(filepath);
      }

      free(filepath);
    }

    else if (msg->type == MSG_TYPE_LIST_SERVER) {
      // chama ls (funçao implementada mais pra cima)
      char *buffer = malloc(sizeof(char) * MAX_PACKAGE_DATA_LENGTH);
      ls(user_dir_path, buffer);

      // envia o resultado de volta para o cliente
      message_t message;
      config_message(&message, MSG_TYPE_LIST_SERVER_RESPONSE, sizeof(buffer), buffer, "");

      char message_buffer[sizeof(message_t)];
      memcpy(message_buffer, &message, sizeof(message));

      int n = sendto(sockfd, message_buffer, sizeof(message_buffer), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
      if (n < 0) {
        printf("ERROR sendto");
        return ERROR;
      }

      free(buffer);
    }

    else if (msg->type == MSG_TYPE_DELETE_ALL) {
      // vai deletar tudo
      printf("Will clean.\n");

      delete_all(user_dir_path);

      // envia o resultado de volta para o cliente
      int n = sendto(sockfd, "ok", 12, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
      if (n < 0) {
        printf("ERROR sendto");
        return ERROR;
      }
    }

    else if (msg->type == MSG_TYPE_GET_ALL) {
      // envia o resultado dizendo que ok
      int n = sendto(sockfd, "ok", 12, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
      if (n < 0) {
        printf("ERROR sendto");
        return ERROR;
      }

      // espera o client dizer que está preparado
  		n = recvfrom(sockfd, message_buffer, sizeof(message_t), 0, (struct sockaddr *) &cli_addr, &clilen);
  		if (n < 0)
  			printf("ERROR on recvfrom");

      // send_all();
      DIR *dir;
      int success = 0;
      struct dirent *ent;

      char *dirpath;
      dirpath = user_dir_path;

      dir = opendir(dirpath);

      if (dir == NULL) {
        return ERROR;
      }

      int j;
      message_t message;
      char message_buffer[sizeof(message_t)];

      // itera sobre os arquivos
      while ((ent = readdir(dir)) != NULL) {
        // para todas as entradas exceto . e ..
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
          char filepath[PATH_MAX_SIZE];
          strcpy(filepath, dirpath);
          strcat(filepath, ent->d_name);

          // envia o nome do arquivo de volta para o cliente
          config_message(&message, MSG_TYPE_DATA, MAX_PACKAGE_DATA_LENGTH, ent->d_name, ent->d_name);
          memcpy(message_buffer, &message, sizeof(message));

          j = sendto(sockfd, message_buffer, sizeof(message_buffer), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
          if (j < 0)
      			printf("ERROR on sendto");

          send_file(filepath);
        }
      }
      closedir (dir);

      // avisa que acabaram os arquivos
      config_message(&message, MSG_END_OF_TRANSMISSION, MAX_PACKAGE_DATA_LENGTH, "", "");
      memcpy(message_buffer, &message, sizeof(message));

      j = sendto(sockfd, message_buffer, sizeof(message_buffer), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
      if (j < 0)
        printf("ERROR on sendto");
    }

    else if (msg->type == MSG_TYPE_LOGOUT) {
      num_connections--;
      printf("%s LOGGED OUT\n", msg->data);
      n = sendto(sockfd, "ok", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
  		if (n  < 0)
  			printf("ERROR on logout");
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
