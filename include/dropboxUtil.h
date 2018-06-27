#ifndef __dropboxUtil__
#define __dropboxUtil__

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <list.h>
#include <process.h>

//--------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------

#define MAXNAME 50
#define MAXFILES 10
#define PATH_MAX_SIZE 150

#define SUCCESS 0
#define ERROR -1

#define FALSE 0
#define TRUE 1

#define BASE_DIR_PATH "sync_dir_"

#define MAX_PACKAGE_DATA_LENGTH 1250 // tamanho maximo dos dados de um pacote a ser enviado

// Tipos de mensagem
#define MSG_TYPE_LOGIN 0
#define MSG_TYPE_SEND_FILE 1
#define MSG_TYPE_GET_FILE 2
#define MSG_TYPE_SYNC 3
#define MSG_TYPE_DATA 4
#define MSG_END_OF_TRANSMISSION 5
#define MSG_TYPE_OK 6
#define MSG_TYPE_LIST_SERVER 7
#define MSG_TYPE_LIST_SERVER_RESPONSE 8
#define MSG_TYPE_DELETE_ALL 9
#define MSG_TYPE_GET_ALL 10
#define MSG_TYPE_LOGOUT 11

// Mensagens internas, entre os processos servidores

#define _MSG_TYPE_CONNECT_PLEASE 100
#define _MSG_TYPE_CONNECTED 101
#define _MSG_TYPE_PLEASE_GIVE_ME_PROCESSESS_DATA 102
#define _MSG_TYPE_PROCESS_DATA 103
#define _MSG_TYPE_END_OF_PROCESS_DATA 104
#define _MSG_TYPE_BACKUP_TO_BACKUP_CONNECT_PLEASE 105
#define _MSG_TYPE_ARE_YOU_OK 106
#define _MSG_TYPE_ELECTION 107
#define _MSG_TYPE_I_AM_THE_LEADER 108
#define _MSG_TYPE_LEADER_HAS_FAILED 109

//--------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------

typedef struct file_info {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
} file_info_t;

typedef struct client {
  int devices[2];
  char userid[MAXNAME];
  file_info_t files[MAXFILES];
  int logged_in;
} client_t;

typedef struct message {
  // int id;                          // identificador do mensagem
  int type;                        // tipo do mensagem
  int size;                        // tamanho da parte de dados do mensagem
  char data[MAX_PACKAGE_DATA_LENGTH];   // parte de dados propriamente dita
  char filename[MAXNAME];
  // file_info_t file;                     // info a respeito do arquivo sendo enviado/recebido
} message_t;


//--------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------

void read_command(char *command, char *argument, int size);
char *build_user_dir_path(char *username);
int dir_exists(char *path);
int read_file_content(char *filename, char *buffer, int start_index, int size);
int write_to_file(char *filename, char *content);
int file_exists(char *file);
void config_message(message_t *message, int type, int size, char *data, char *filename);
int ls(char *dirpath, char *buffer);
int delete_all(char *dirpath);
int create_socket(char *host, int port, struct sockaddr_in *server_address);
int send_message2(int socket_id, message_t message, struct sockaddr_in *server_address);
int receive_message2(int socket_id, char *buffer, int size);
void config_message2(message_t *message, int type, int size, char *data, char *filename);
void get_local_ip(char *buffer);
int broadcast_message(message_t *m, list_t **other_processes, int pid);

#endif
