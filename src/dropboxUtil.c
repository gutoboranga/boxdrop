#include <dropboxUtil.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <list.h>
#include <process.h>

void read_command(char *command, char *argument, int size) {
  fgets(command, size, stdin);

  int last_index = strlen(command) - 1;

  // remove \n se foi colocado no final pelo fgets
  if (command[last_index] == '\n') {
    command[last_index] = 0;
  }

  // salva em command a primeira palavra da string, separada por espaços
  strtok(command, " ");

  // salva em aux um ponteiro pra segunda parte do comando (argumento)
  char *aux = strtok(NULL, " ");
  if (aux == NULL) {
    // se não há argumento, bota "" em aux (argument não pode receber NULL)
    aux = "";
  }

  // copia de aux pra argument
  strcpy(argument, aux);
}

char *build_user_dir_path(char *user) {
  char *path;

  path = malloc(sizeof(char) * PATH_MAX_SIZE);
  strcpy(path, BASE_DIR_PATH);
  strcat(path, user);

  return path;
}

int dir_exists(char *path) {
  struct stat st = {0};

  if (stat(path, &st) == -1) {
      return FALSE;
  }

  return TRUE;
}

int read_file_content(char *filename, char *buffer, int start_index, int size) {
  FILE *file = fopen(filename, "r");

  if (!file) {
    return ERROR;
  }

  fseek(file, start_index, SEEK_SET);
  int amount_read = fread(buffer, 1, size, file);
  fclose(file);

  return amount_read;
}

int write_to_file(char *filename, char *content) {
  FILE *file = fopen(filename, "a+");

  if (!file) {
    return ERROR;
  }

  fprintf(file, "%s", content);
  fclose(file);

  return SUCCESS;
}

int file_exists(char *filename) {
  FILE *file = fopen(filename, "r");

  if (!file) {
    return FALSE;
  }

  fclose(file);
  return TRUE;
}

void config_message(message_t *message, int type, int size, char *data, char *filename) {
  message->type = type;
  message->size = size;
  memcpy(message->data, data, MAX_PACKAGE_DATA_LENGTH);
  strcpy(message->filename, filename);
}

int ls(char *dirpath, char *buffer) {
  DIR *dir;
  dir = opendir(dirpath);

  if (dir == NULL) {
    return ERROR;
  }

  struct dirent *ent;
  int buffer_index = 0;

  strcpy(buffer, "");

  while ((ent = readdir(dir)) != NULL) {
    // salva no buffer todas as entradas exceto . e ..
    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
      strcpy(buffer + buffer_index, "  ");
      buffer_index += 2;

      strcpy(buffer + buffer_index, ent->d_name);
      buffer_index += strlen(ent->d_name);

      struct tm *t;
      struct stat attrib;

      char full_path[PATH_MAX_SIZE];
      strcpy(full_path, dirpath);
      strcat(full_path, "/");
      strcat(full_path, ent->d_name);

      stat(full_path, &attrib);
      t = localtime(&(attrib.st_mtime));
      char mod_time[32];
      sprintf(mod_time, "\t - %d/%d/%d_%d:%d:%d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
      strcpy(buffer + buffer_index, mod_time);
      buffer_index += strlen(mod_time);

      memcpy(buffer + buffer_index, "\n", 1);
      buffer_index += 1;
    }
  }
  closedir (dir);

  return SUCCESS;
}

int delete_all(char *dirpath) {
  DIR *dir;
  int success = 0;
  struct dirent *ent;

  dir = opendir(dirpath);

  if (dir == NULL) {
    return ERROR;
  }

  // aí sim itera sobre os arquivos
  while ((ent = readdir(dir)) != NULL) {
    // para todas as entradas exceto . e ..
    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
      char filepath[PATH_MAX_SIZE];
      strcpy(filepath, dirpath);
      strcat(filepath, "/");
      strcat(filepath, ent->d_name);

      if (remove(filepath) != SUCCESS) {
        success = -1;
      }
    }
  }
  closedir (dir);

  return success;
}

int create_socket(char *host, int port, struct sockaddr_in *server_address) {
  struct hostent *server;
  // struct sockaddr_in s_address;
  
	server = gethostbyname(host);
  
	if (server == NULL) {
    return ERROR;
  }

  // cria o socket
  int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
  
  // seta umas paradas do servidor
  server_address->sin_family = AF_INET;
	server_address->sin_port = htons(port);
	server_address->sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(server_address->sin_zero), 8);

  return socket_id;
}

int send_message2(int socket_id, message_t message, struct sockaddr_in *server_address) {
  char message_buffer[sizeof(message_t)];

  // "serializa" a struct mensagem (como um buffer) para enviar pelo socket
  memcpy(message_buffer, &message, sizeof(message));

  int n = sendto(socket_id, message_buffer, sizeof(message_buffer), 0, (const struct sockaddr *) server_address, sizeof(struct sockaddr_in));
  if (n < 0) {
    printf("ERROR sendto\n");
    return ERROR;
  }
  return SUCCESS;
}

int receive_message2(int socket_id, char *buffer, int size) {
  struct sockaddr_in from;
  unsigned int length = sizeof(struct sockaddr_in);

  int n = recvfrom(socket_id, buffer, size, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    printf("ERROR recvfrom\n");
    return ERROR;
  }

  return SUCCESS;
}

void config_message2(message_t *message, int type, int size, char *data, char *filename) {
  message->type = type;
  message->size = size;
  memcpy(message->data, data, MAX_PACKAGE_DATA_LENGTH);
  memcpy(message->filename, filename, MAXNAME);
}

void get_local_ip(char *buffer) {
  
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *hostentry;
    int hostname;
 
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname == -1) buffer = NULL;
 
    // To retrieve host information
    hostentry = gethostbyname(hostbuffer);
    if (hostentry == NULL) buffer = NULL;
 
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*) hostentry->h_addr_list[0]));
    
    strcpy(buffer, IPbuffer);
}

//
// broadcast_message
//
// envia a mensagem m para todos processos com pid maior que o int pid
// retorna o número de processos pros quais enviou alerta
//
int broadcast_message(message_t *m, list_t **other_processes, int pid) {
  list_t *aux;
	process_t *p;
  
  int sent_count = 0;
  
  aux = *(other_processes);

	while (aux != NULL) {
		p = (process_t *) aux->value;

    // se o pid do processo for maior que o recebido por parâmetro
    if (p->pid > pid && p->role != PRIMARY) {
      send_message2(p->socket_id, *m, &(p->address));
      sent_count += 1;
    }
    
		aux = aux->next;
	}
  
  return sent_count;
}
