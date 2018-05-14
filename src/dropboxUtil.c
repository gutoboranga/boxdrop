#include <dropboxUtil.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

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

void config_message(message_t *message, int type, int size, char *data, char *filename) {
  message->type = type;
  message->size = size;
  memcpy(message->data, data, MAX_PACKAGE_DATA_LENGTH);
  strcpy(message->filename, filename);
}

int ls(char *dirpath, char *buffer) {
  DIR *dir;
  dir = opendir(dirpath);
  
  if (dir == NULL) {
    return ERROR;
  }
  
  struct dirent *ent;
  int buffer_index = 0;
  
  strcpy(buffer, "");
  
  while ((ent = readdir(dir)) != NULL) {
    // salva no buffer todas as entradas exceto . e ..
    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
      strcpy(buffer + buffer_index, "  ");
      buffer_index += 2;
      
      strcpy(buffer + buffer_index, ent->d_name);
      buffer_index += strlen(ent->d_name);
  
      memcpy(buffer + buffer_index, "\n", 1);
      buffer_index += 1;
    }
  }
  closedir (dir);
  
  return SUCCESS;
}

int delete_all(char *dirpath) {
  DIR *dir;
  int success = 0;
  struct dirent *ent;
  
  dir = opendir(dirpath);
  
  if (dir == NULL) {
    return ERROR;
  }
  
  // aí sim itera sobre os arquivos
  while ((ent = readdir(dir)) != NULL) {
    // para todas as entradas exceto . e ..
    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
      char filepath[PATH_MAX_SIZE];
      strcpy(filepath, dirpath);
      strcat(filepath, "/");
      strcat(filepath, ent->d_name);
      
      if (remove(filepath) != SUCCESS) {
        success = -1;
      }
    }
  }
  closedir (dir);
  
  return success;
}

