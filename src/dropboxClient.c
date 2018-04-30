#include <dropboxClient.h>
#include <dropboxUtil.h>
#include <sys/stat.h>
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

void get_sync_dir(char *user) {
  // cria uma string com o path completo "/home/sync_dir_<USER NAME>"
  char *path = build_user_dir_path(user);
  
  // se não existe o dir ainda, cria
  if(!dir_exists(path)) {
    mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
  }
  
  // let it go
  free(path);
}

int main(int argc, char *argv[]) {
  if(argc != CLIENT_PARAMS_NUMBER) {
      printf(CLIENT_INVALID_PARAMS_NUMBER);
      return ERROR;
   }
   
   char *user = argv[1];
   char *adress = argv[2];
   int door = atoi(argv[3]);
   
   // Tenta conectar com o servidor
   if (login_server(adress, door) == ERROR) {
     // printf(CLIENT_ERROR_LOGGING_IN, adress, door);
     return ERROR;
   }
   
   // Chamada para ver se o diretorio do usuario existe na máquina local
   // do client (se não, o cria) e sincronizar com o servidor
   get_sync_dir(user);
   
   // Início da linha de comando para o usuário:
   // fica lendo os comandos até rolar um exit, daí acaba tudo
   char *command = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   char *argument = malloc(sizeof(char) * CLIENT_COMMAND_MAX_SIZE);;
   
   while(1) {
     printf("> ");
     
     read_command(command, argument, CLIENT_COMMAND_MAX_SIZE);
     
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
   
   free(command);
   free(argument);
   
   return SUCCESS;
}
