#ifndef __dropboxServer__
#define __dropboxServer__

#include <dropboxUtil.h>

void sync_server();
void receive_file(char *file);
int send_file(char *file);

//--------------------------------------------------------------------------------
// Messages
//--------------------------------------------------------------------------------

#define SERVER_FILE_NOT_FOUND "  [ERRO] O arquivo requisitado não foi encontrado no servidor.\n"
#define MAX_CONNECTIONS 2


#endif
