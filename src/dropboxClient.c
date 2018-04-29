#include <dropboxClient.h>
#include <dropboxUtil.h>
#include <stdio.h>

int login_server(char *host, int port) {
  return -1;
}

void sync_client() {
}

void send_file(char *file) {
}

void get_file(char *file) {
}

void delete_file(char *filet) {
}

void close_session() {
}

int main(int argc, char *argv[]) {
  if(argc != CLIENT_PARAMS_NUMBER) {
      printf(CLIENT_INVALID_PARAMS_NUMBER);
      return 0;
   }
   
   // conecta com o servidor, provavelmente através de
   // login_server();
   
   // chama get_sync_dir().
   // o servidor verificará se o diretório “sync_dir_<nomeusuário>”
   // existe no dispositivo do cliente, e criá-lo se necessário.
   // A sincronização dos arquivos deverá ser efetuada
   
   
   
   return 0;
}