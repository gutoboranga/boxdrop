#ifndef __dropboxUtil__
#define __dropboxUtil__

//--------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------

#define MAXNAME 50
#define MAXFILES 10

#define SUCCESS 0
#define ERROR -1

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

void readCommand(char *command, char *argument, int size);

#endif
