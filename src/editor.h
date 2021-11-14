#ifndef EDITOR_H
#define EDITOR_H

#include "abuf.h"

#include <stdbool.h>
#include <termios.h>
#include <time.h>

// Mask to imitate a CTRL key press on keyboard.
#define CTRL_KEY(k) ((k)&0x1f)

#define TAB_SIZE 4

enum key
{
  BACKSPACE = 127,
  ARROW_UP = 1000,
  ARROW_DOWN,
  ARROW_LEFT,
  ARROW_RIGHT,
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
} e_row;

struct editor_config
{
  int cursor_x, cursor_y;
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

extern struct editor_config E;

int editor_read_key ();

/************************ row operations ********************/

int editor_convert_cx_to_rx (e_row *row, const int cx);

void editor_update_row (e_row *row);

void editor_insert_row (int at, char *s, size_t len);

void editor_append_row (char *s, size_t len);

void editor_row_insert_char (e_row *row, int at, int c);

void editor_row_delete_char (e_row *row, int at);

void editor_row_append_string (e_row *row, char *str, size_t length);

void editor_delete_row (int at);

/************************ Editor operations ********************/

void editor_insert_char (char c);

void editor_delete_char ();

void editor_insert_newline ();

/************************ file i/o ********************/

void editor_open (const char *file_name);

// LEAK WARNING: the NEW_BUFFER is expected to free by caller
char *editor_rows_to_string (int *buffer_length);

bool editor_save ();

/************************* output ****************************/

void editorScroll ();

void editor_draw_rows (struct abuf *ab);

void editor_draw_status_bar (struct abuf *ab);

void editor_draw_message_bar (struct abuf *ab);

void editor_refresh_screen ();

void editor_set_status_message (const char *fmt, ...);

void editor_navigate_cursor (int key);

/************************ input ***********************/

void editor_navigate_cursor (int key);

void editor_process_keypress ();

/************************ init ***********************/
void init_editor ();

#endif