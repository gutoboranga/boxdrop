#ifndef __dropboxServerFront__
#define __dropboxServerFront__

#include <dropboxUtil.h>

void sync_server();
void receive_file(char *file);
int send_file(char *file);
void *handle_client_requests(void *process);

//--------------------------------------------------------------------------------
// Messages
//--------------------------------------------------------------------------------

#define SERVER_FILE_NOT_FOUND "  [ERRO] O arquivo requisitado não foi encontrado no servidor.\n"
#define MAX_CONNECTIONS 2


#endif
