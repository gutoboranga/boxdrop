#ifndef __dropboxClient__
#define __dropboxClient__

#include <dropboxUtil.h>

int login_server(char *host, int port);
void sync_client();
int send_file(char *file);
void get_file(char *file);
void delete_file(char *file);
void close_session ();

//--------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------

#define CLIENT_PARAMS_NUMBER 4
#define CLIENT_COMMAND_MAX_SIZE 100

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

#define CLIENT_INVALID_PARAMS_NUMBER "\n[ERRO] Número inválido de argumentos. O uso correto é:\n\n\t$ ./bin/dropboxClient usuario endereco porta\n\n"
#define CLIENT_ERROR_LOGGING_IN "\n[ERRO] Não foi possível conectar com o servidor %s na porta %i\n\n"

#define CLIENT_INVALID_COMMAND "\n[ERRO] Comando inválido.\n"
#define CLIENT_COMMANDS_HELP "Os comandos disponíveis são:\n\n\t> upload <path/filename.ext>\n\t> download <filename.ext>\n\t> list_server\n\t> list_client\n\t> get_sync_dir\n\t> exit\n\n"

#define CLIENT_UPLOAD_CMD_MISSING_ARGUMENT "\n[ERRO] O comando upload deve receber um parâmetro:\n\n\t> upload <path/filename.ext>\n\n"
#define CLIENT_DOWNLOAD_CMD_MISSING_ARGUMENT "\n[ERRO] O comando download deve receber um parâmetro:\n\n\t> download <filename.ext>\n\n"

#define CLIENT_UPLOAD_SUCCESS "  Arquivo %s enviado com sucesso!\n"

#define CLIENT_UPLOAD_NO_SUCH_FILE "  [ERRO] O arquivo %s não foi encontrado.\n"

#endif
