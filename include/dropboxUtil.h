#ifndef __dropboxUtil__
#define __dropboxUtil__

//--------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------

#define MAXNAME 50
#define MAXFILES 10
#define PATH_MAX_SIZE 150

#define SUCCESS 0
#define ERROR -1

#define FALSE 0
#define TRUE 1

#define BASE_DIR_PATH "/home/sync_dir_"

//--------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------

typedef struct file_info {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
} file_info_t;

typedef struct client {
  int devices[2];
  char userid[MAXNAME];
  file_info_t files[MAXFILES];
  int logged_in;
} client_t;

//--------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------

void read_command(char *command, char *argument, int size);
char *build_user_dir_path(char *username);
int dir_exists(char *path);

#endif
