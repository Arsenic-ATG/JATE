#ifndef FUNC_PROTOTYPE
#define FUNC_PROTOTYPE

#include "global_var.h"

void editor_set_status_message (const char *fmt, ...);

/***************** error handling ************************/

void die(const char *s);

/***************** terminal *****************************/

void disable_raw_mode ();
void enable_raw_mode ();
int editor_read_key();
int get_cursor_position(int *rows,int *cols);
int get_windows_size(int *rows, int *cols);

/************************ row operations ********************/

int editor_convert_cx_to_rx(e_row *row,const int cx);
void editor_update_row(e_row *row);
void editor_insert_row(int at, char *s, size_t len);
void editor_append_row(char *s, size_t len) ;
void editor_row_insert_char(e_row *row, int at, int c);
void editor_row_delete_char(e_row *row, int at);
void editor_row_append_string(e_row *row, char *str, size_t length);
void editor_delete_row(int at);

/************************ Editor operations ********************/

void editor_insert_char(char c);
void editor_delete_char();
void editor_insert_newline();

/************************ file i/o ********************/

void editor_open(const char* file_name);
char *editor_rows_to_string(int *buffer_length);
bool editor_save();

/************************ append buffer ********************/

void ab_append(struct abuf *ab,const char *str,int len);
void ab_free(struct abuf *ab);

/************************* output ****************************/

void editorScroll(); 
void editor_draw_rows(struct abuf *ab);
void editor_draw_status_bar(struct abuf *ab);
void editor_draw_message_bar(struct abuf *ab);
void editor_refresh_screen();

/************************ input ***********************/

void editor_navigate_cursor(int key);
void editor_process_keypress();

/************************** init *************************/

void init_editor();

#endif