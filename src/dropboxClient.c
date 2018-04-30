#include <dropboxClient.h>
#include <dropboxUtil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int login_server(char *host, int port) {
  // a ser implementada
  return SUCCESS;
}

void sync_client() {
}

void send_file(char *file) {
}

void get_file(char *file) {
}

void delete_file(char *file) {
}

void close_session() {
}

int main(int argc, char *argv[]) {
  if(argc != CLIENT_PARAMS_NUMBER) {
      printf(CLIENT_INVALID_PARAMS_NUMBER);
      return ERROR;
   }
   
   char *adress = argv[2];
   int door = atoi(argv[3]);
   
   // tenta conectar com o servidor
   if (login_server(adress, door) == ERROR) {
     // printf(CLIENT_ERROR_LOGGING_IN, adress, door);
     return ERROR;
   }
   
   // chama get_sync_dir().
   // o servidor verificará se o diretório “sync_dir_<nomeusuário>”
   // existe no dispositivo do cliente, e criá-lo se necessário.
   // A sincronização dos arquivos deverá ser efetuada
   
   char *command = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   char *argument = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   
   // entra num loop de ficar lendo os comandos do usuário até
   // ele mandar um exit, daí acaba tudo
   while(1) {
     printf("> ");
     
     readCommand(command, argument, CLIENT_COMMAND_MAX_SIZE);
     
     // confere cada possível comando:
     if (strcmp(command, CLIENT_EXIT_CMD) == 0) {
       // faz logout do sistema e retorna
       return SUCCESS;
     }
     
     else if (strcmp(command, CLIENT_UPLOAD_CMD) == 0) {
       // garante que argument não é inválido
       if (strcmp(argument, "") == 0) {
         printf(CLIENT_UPLOAD_CMD_MISSING_ARGUMENT);
         continue;
       }
       
       // aqui sim faz o que deve ser feito
     }
     
     else if (strcmp(command, CLIENT_DOWNLOAD_CMD) == 0) {
       // garante que argument não é inválido
       if (strcmp(argument, "") == 0) {
         printf(CLIENT_DOWNLOAD_CMD_MISSING_ARGUMENT);
         continue;
       }
       
       // aqui sim faz o que deve ser feito
     }
     
     else if (strcmp(command, CLIENT_LIST_SERVER_CMD) == 0) {
     }
     
     else if (strcmp(command, CLIENT_LIST_CLIENT_CMD) == 0) {
     }
     
     else if (strcmp(command, CLIENT_GET_SYNC_DYR_CMD) == 0) {
     }
     
     else {
       printf(CLIENT_INVALID_COMMAND);
       printf(CLIENT_COMMANDS_HELP);
     }
   }
   
   return SUCCESS;
}
