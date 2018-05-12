#ifndef __dropboxUtil__
#define __dropboxUtil__

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

#define BASE_DIR_PATH "/home/sync_dir_"

#define MAX_PACKAGE_DATA_LENGTH 128 // tamanho maximo dos dados de um pacote a ser enviado

// Tipos de mensagem
#define MSG_TYPE_SEND_FILE 0
#define MSG_TYPE_GET_FILE 1
#define MSG_TYPE_SYNC 2
#define MSG_TYPE_DATA 3
#define MSG_END_OF_TRANSMISSION 4

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

// header_message representa a primeira de uma série de mensagens.
// Serve por exemplo, para especificar ao servidor que tipo de serviço o cliente requer.
typedef struct header_message {
  int type;                             // tipo da mensagem
  file_info_t file;                     // info a respeito do arquivo sendo enviado/recebido
} header_message_t;

// data_message representa a uma mensagem com dados puros.
typedef struct data_message {
  int id;                               // identificador do mensagem
  int size;                             // tamanho válido da parte de dados do mensagem
  char data[MAX_PACKAGE_DATA_LENGTH];   // parte de dados propriamente dita
} data_message_t;


//--------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------

void read_command(char *command, char *argument, int size);
char *build_user_dir_path(char *username);
int dir_exists(char *path);
int read_file_content(char *filename, char *buffer, int start_index, int size);
int write_to_file(char *filename, char *content);
int file_exists(char *file);

#endif
