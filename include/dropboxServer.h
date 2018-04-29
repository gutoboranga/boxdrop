#ifndef __dropboxServer__
#define __dropboxServer__

#include <dropboxUtil.h>

void sync_server();
void receive_file(char *file);
void send_file(char *file);

#endif
