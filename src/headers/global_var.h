#ifndef GLOBAL_VAR
#define GLOBAL_VAR

#include "includes.h"

enum key
{
  BACKSPACE = 127,
  ARROW_UP = 1000,
  ARROW_DOWN ,
  ARROW_LEFT ,
  ARROW_RIGHT ,
  // Bellow keys are yet to be implemented
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN,
};

typedef struct editor_row 
{
  int size;
  char *text;
  int r_size;
  char *renderer;
}e_row;

struct editor_config
{
  int cursor_x,cursor_y;
  int renderer_x;
  int screen_rows;
  int screen_cols;
  int num_rows;
  int row_offset;
  int col_offset;
  e_row *row;
  bool modified;
  char *filename;
  char status_msg[80];
  time_t status_msg_time;
  struct termios orig_termios;
};

struct editor_config E;

struct abuf 
{
  char *b;
  int len;
};

#endif
