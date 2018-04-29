#ifndef __dropboxClient__
#define __dropboxClient__

#include <dropboxUtil.h>

int login_server(char *host, int port);
void sync_client();
void send_file(char *file);
void get_file(char *file);
void delete_file(char *file);
void close_session ();

//--------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------

#define CLIENT_PARAMS_NUMBER 4

//--------------------------------------------------------------------------------
// Commands
//--------------------------------------------------------------------------------

#define CLIENT_UPLOAD_CMD "upload"
#define CLIENT_DOWNLOAD_CMD "download"
#define CLIENT_LIST_SERVER_CMD "list_server"
#define CLIENT_LIST_CLIENT_CMD "list_client"
#define CLIENT_GET_SYNC_DYR_CMD "get_sync_dir"
#define CLIENT_EXIT_CMD "exit"

//--------------------------------------------------------------------------------
// Messages
//--------------------------------------------------------------------------------

#define CLIENT_INVALID_PARAMS_NUMBER "\n[ERRO] Número inválido de argumentos. O uso correto é:\n\n\t$ ./bin/dropboxClient user endereco porta\n\n"
#define CLIENT_INVALID_COMMAND "\n[ERRO] Comando inválido.\n"
#define CLIENT_COMMANDS_HELP "Os comandos disponíveis são:\n\n\t> upload <path/filename.ext>\n\t> download <filename.ext>\n\t> list_server\n\t> list_client\n\t> get_sync_dir\n\t> exit\n\n"

#endif
