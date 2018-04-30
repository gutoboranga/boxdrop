#include <dropboxUtil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void readCommand(char *command, char *argument, int size) {
  fgets(command, size, stdin);
  
  int last_index = strlen(command) - 1;
  
  // remove \n se foi colocado no final pelo fgets
  if (command[last_index] == '\n') {
    command[last_index] = 0;
  }
  
  // salva em command a primeira palavra da string, separada por espaços
  strtok(command, " ");
  
  // salva em aux um ponteiro pra segunda parte do comando (argumento)
  char *aux = strtok(NULL, " ");
  if (aux == NULL) {
    // se não há argumento, bota "" em aux (argument não pode receber NULL)
    aux = "";
  }
  
  // copia de aux pra argument
  strcpy(argument, aux);
}
