#include <dropboxUtil.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void read_command(char *command, char *argument, int size) {
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

char *build_user_dir_path(char *user) {
  char *path;
  
  path = malloc(sizeof(char) * PATH_MAX_SIZE);
  strcpy(path, BASE_DIR_PATH);
  strcat(path, user);
  
  return path;
}

int dir_exists(char *path) {
  struct stat st = {0};

  if (stat(path, &st) == -1) {
      return FALSE;
  }
  
  return TRUE;
}

int read_file_content(char *filename, char *buffer, int start_index, int size) {
  FILE *file = fopen(filename, "r");
  
  if (!file) {
    return ERROR;
  }
  
  fseek(file, start_index, SEEK_SET);
  int amount_read = fread(buffer, 1, size, file);
  fclose(file);
  
  return amount_read;
}

int write_to_file(char *filename, char *content) {
  FILE *file = fopen(filename, "a+");
  
  if (!file) {
    return ERROR;
  }
  
  fprintf(file, "%s", content);
  fclose(file);
  
  return SUCCESS;
}

int file_exists(char *filename) {
  FILE *file = fopen(filename, "r");
  
  if (!file) {
    return FALSE;
  }
  
  fclose(file);
  return TRUE;
}
