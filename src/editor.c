/****************** macros *************************/

// feature test macros
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

// Mask to imitate a CTRL key press on keyboard.
#define CTRL_KEY(k) ((k) & 0x1f)

#define TAB_SIZE 4

/****************** headers *************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>

/***************** global variables **********************/

enum key
{
  BACKSPACE = 127,
  ARROW_UP = 1000,
  ARROW_DOWN ,
  ARROW_LEFT ,
  ARROW_RIGHT ,
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

/***************** error handling ************************/

void die(const char *s)
{
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}
/***************** function Prototypes ******************/

// TODO: possibly seperate them out in different headers.
void editor_set_status_message (const char *fmt, ...);

/***************** terminal *****************************/
void disable_raw_mode ()
{
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enable_raw_mode ()
{
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");

  atexit(disable_raw_mode);

  // Make sure that the changes don't take place globally.
  struct termios raw = E.orig_termios;

  // Toggle canonical mode off and also problem tracking with SIGCONT and SIGSTP.
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // To set up timeout for input.
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcgetattr");
}

int editor_read_key()
{
    char c;
    int charaters_read = read(STDIN_FILENO, &c, 1);

    while(charaters_read != 1)
      {
        charaters_read = read(STDIN_FILENO, &c, 1);
        if( charaters_read == -1)
          die("read");
      }

    // read escape sequence
    if (c == '\x1b')
      {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
          return '\x1b';

        if (read(STDIN_FILENO, &seq[1], 1) != 1)
          return '\x1b';

        if (seq[0] == '[')
          {
            if (seq[1] >= '0' && seq[1] <= '9')
              {

                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                  return '\x1b';

                if (seq[2] == '~')
                  {
                    switch (seq[1])
                      {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                      }
                  }
              }

            else
              {
                switch (seq[1])
                  {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                  }
              }
          }
        else if (seq[0] == 'O')
          {
            switch (seq[1])
              {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
              }
          }

        return '\x1b';
      }

    else
      return c;
}

int get_cursor_position(int *rows,int *cols)
{
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while(i < sizeof(buf) - 1)
    {
      if (read(STDIN_FILENO,&buf[i],1) != 1)
        break;
      else if (buf[i] == 'R')
        break;
      i++;
    }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int get_windows_size(int *rows, int *cols)
{
  struct winsize ws;

  if(ioctl(STDOUT_FILENO , TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
      if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
        return -1;

      return get_cursor_position(rows,cols);
    }
  else
    {
      // Return the current screen size of the terminal window.
      *rows = ws.ws_row;
      *cols = ws.ws_col;
      return 0;
    }
}

/************************ row operations ********************/

int editor_convert_cx_to_rx(e_row *row,const int cx)
{
  int rx = 0;

  for (int j = 0; j < cx; j++)
    {
      if (row->text[j] == '\t')
        rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);

      rx++;
    }

  return rx;
}

void editor_update_row(e_row *row)
{
  int tabs = 0;
  // Find tabs and allocate required amount of memory dynamically.
  for (int j = 0; j < row->size; j++)
    if (row->text[j] == '\t')
      tabs++;

  free(row->renderer);
  row->renderer = malloc(row->size + (tabs * (TAB_SIZE - 1)) + 1);

  int idx = 0;
  for (int j = 0; j < row->size; j++)
    {
      if(row->text[j] == '\t')
        {
          row->renderer[idx++] = ' ';
          while(idx % TAB_SIZE != 0)
            row->renderer[idx++] = ' ';
        }
      else
        {
          row->renderer[idx++] = row->text[j];
        }
    }

  row->renderer[idx] = '\0';
  row->r_size = idx;
}

void editor_insert_row(int at, char *s, size_t len)
{
  if(at < 0 || at > E.num_rows)
    return;

  E.row = realloc(E.row, sizeof(e_row) * (E.num_rows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(e_row) * (E.num_rows - at));

  E.row[at].size = len;
  E.row[at].text = malloc(len + 1);
  memcpy(E.row[at].text, s, len);
  E.row[at].text[len] = '\0';

  E.row[at].r_size = 0;
  E.row[at].renderer = NULL;
  editor_update_row(&E.row[at]);

  E.num_rows++;
  E.modified = 1;
}

void editor_append_row(char *s, size_t len)
{
  int at = E.num_rows;
  editor_insert_row(at, s, len);
}

void editor_row_insert_char(e_row *row, int at, int c)
{
  if (at < 0 || at > row->size)
    at = row->size;
  row->text = realloc(row->text, row->size + 2);
  memmove(&row->text[at + 1], &row->text[at], row->size - at + 1);
  row->size++;
  row->text[at] = c;
  editor_update_row(row);
  E.modified = 1;
}

void editor_row_delete_char(e_row *row, int at)
{
  if (at < 0 || at >= row->size)
    return;
  memmove(&row->text[at], &row->text[at + 1], row->size - at + 1);
  row->size--;
  editor_update_row(row);
  E.modified = 1;
}

void editor_row_append_string(e_row *row, char *str, size_t length)
{
  row->text = realloc(row->text, row->size + length + 1);
  memcpy(&row->text[row->size], str, length);
  row->size += length;
  row->text[row->size] = '\0';
  editor_update_row(row);
  E.modified = 1;
}

void editor_delete_row(int at)
{
  if (at < 0 || at >= E.num_rows)
    return;
  // free row
  free(E.row[at].renderer);
  free(E.row[at].text);
  memmove(&E.row[at], &E.row[at + 1], sizeof(e_row) * (E.num_rows - at - 1));
  E.num_rows--;
  E.modified = 1;
}

/************************ Editor operations ********************/

void editor_insert_char(char c)
{
  // The the cursor is at the end of file
  if(E.cursor_y == E.num_rows)
    editor_append_row("",0);

  editor_row_insert_char(&E.row[E.cursor_y], E.cursor_x, c);
  E.cursor_x++;
}

void editor_delete_char(int key)
{
  if (E.cursor_y == E.num_rows)
    return;

  if (key == BACKSPACE || key == CTRL_KEY('h')) {
    if (E.cursor_y == 0 && E.cursor_x == 0)
      return;

    if (E.cursor_x > 0) {
      editor_row_delete_char(&E.row[E.cursor_y], E.cursor_x - 1);
      E.cursor_x--;
    }
    else {
      E.cursor_x = E.row[E.cursor_y - 1].size;
      editor_row_append_string(&E.row[E.cursor_y - 1],
                               E.row[E.cursor_y].text,
                               E.row[E.cursor_y].size);
      editor_delete_row(E.cursor_y);
      E.cursor_y--;
    }
  } else if (key == DEL_KEY) {
    // If the cursor is at the start of the row and the row is empty,
    // move the row below to current row.
    if (E.cursor_y >= 0 && E.cursor_x == 0 &&
	    E.row[E.cursor_y].text[0] == '\0') {

      // Stay within bounds, prevent undefined behavior.
      if (E.cursor_y == E.num_rows - 1)
        return;

      editor_row_append_string(&E.row[E.cursor_y],
                               E.row[E.cursor_y + 1].text,
                               E.row[E.cursor_y + 1].size);
      editor_delete_row(E.cursor_y + 1);
    }
    else {
      editor_row_delete_char(&E.row[E.cursor_y], E.cursor_x);
    }
  }
}

void editor_insert_newline()
{
  if(E.cursor_x == 0)
    editor_insert_row(E.cursor_y, "", 0);
  else
    {
      e_row *row = &E.row[E.cursor_y];
      editor_insert_row(E.cursor_y + 1, &row->text[E.cursor_x],
                        row->size - E.cursor_x);

      // reassigning pointer as editor_insert_row() reallocates E.row
      row = &E.row[E.cursor_y];
      row->size = E.cursor_x;
      row->text[row->size] = '\0';
      editor_update_row(row);
    }

  E.cursor_y++;
  E.cursor_x = 0;
}

/************************ file i/o ********************/

void editor_open(const char* file_name)
{
  FILE *fp = fopen(file_name, "r");
  E.filename = strdup(file_name);

  if (!fp)
    die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  linelen = getline(&line, &linecap, fp);

  while( linelen != -1 )
  {
    while (linelen > 0 && (line[linelen - 1] == '\n'
           || line[linelen - 1] == '\r'))
      linelen--;

    editor_append_row(line, linelen);

    linelen = getline(&line, &linecap, fp);
  }

  free(line);
  fclose(fp);
  E.modified = 0;
}

// LEAK WARNING: the NEW_BUFFER is expected to free by caller
char *editor_rows_to_string(int *buffer_length)
{
  int total_length = 0;
  for (int i = 0; i < E.num_rows; i++)
    total_length += E.row[i].size + 1;
  *buffer_length = total_length;

  char *new_buffer = malloc(total_length);
  char *p = new_buffer;

  for (int i = 0; i < E.num_rows; i++)
  {
    memcpy(p, E.row[i].text, E.row[i].size);
    p += E.row[i].size;
    *p = '\n';
    p++;
  }

  return new_buffer;
}

bool editor_save()
{
  // TODO: Handle the case where the file is not provided in the begining.
  if (E.filename == NULL)
    return false;

  int length;
  char *buffer = editor_rows_to_string(&length);
  int file_descriptor = open(E.filename, O_RDWR | O_CREAT, 0644);

  // TODO: perform some error handling where saving fails.
  if (file_descriptor != -1)
    {
      if (ftruncate(file_descriptor, length) != -1)
        {
          if (write(file_descriptor, buffer, length) == length)
            {
              free(buffer);
              close(file_descriptor);
              return true;
            }
        }
      close(file_descriptor);
    }

  // Cleanup
  free(buffer);
  return false;
}

/************************ append buffer ********************/

struct abuf
{
  char *b;
  int len;
};

// constructor
#define ABUF_INIT {NULL, 0}

void ab_append(struct abuf *ab,const char *str,int len)
{
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL)
    return;

  memcpy(&new[ab->len], str, len);
  ab->b = new;
  ab->len += len;
}

// desctructor
void ab_free(struct abuf *ab)
{
  free(ab->b);
}

/************************* output ****************************/

void editorScroll()
{
  E.renderer_x = 0;

  if (E.cursor_y < E.num_rows)
    E.renderer_x = editor_convert_cx_to_rx(&E.row[E.cursor_y], E.cursor_x);

  if (E.cursor_y < E.row_offset)
    E.row_offset = E.cursor_y;

  if (E.cursor_y >= E.row_offset + E.screen_rows)
    E.row_offset = E.cursor_y - E.screen_rows + 1;

  if (E.renderer_x < E.col_offset)
    E.col_offset = E.renderer_x;

  if (E.renderer_x >= E.col_offset + E.screen_cols)
    E.col_offset = E.renderer_x - E.screen_cols + 1;

}

// using append buffer to paint to prevent flickring while typing
void editor_draw_rows(struct abuf *ab)
{
  int y;
  for (y = 0; y < E.screen_rows; y++)
    {
      int filerow = y + E.row_offset;
      if(filerow >= E.num_rows)
        {
          // Welcome mesage
          if (E.num_rows == 0 && y == (E.screen_rows / 8))
            {
              char welcome_buffer[60];
              int message_length
                = snprintf (welcome_buffer, sizeof(welcome_buffer),
                            "Welcome the the Text Editor " );

              if (message_length > E.screen_cols)
                message_length = E.screen_cols;

              // Subracting one for first ">" character
              int spacing = ((E.screen_cols - message_length)/2) - 1;
              ab_append(ab,">",1);
              for (int i = 0; i < spacing; ++i)
                {
                  ab_append(ab," ",1);
                }

              ab_append(ab,welcome_buffer,message_length);
            }
          else
            {
              // start of new line ( Should this be replaced with line number ?)
              ab_append(ab, ">", 1);
            }
        }

      else
        {
          int len = E.row[filerow].r_size - E.col_offset;

          if(len < 0)
            len = 0;

          if (len > E.screen_cols)
            len = E.screen_cols;

          ab_append(ab, &E.row[filerow].renderer[E.col_offset], len);
        }

      // Clear "in line"
      ab_append(ab, "\x1b[K", 3);
      ab_append(ab, "\r\n", 2);
    }
}

void editor_draw_status_bar(struct abuf *ab)
{
  ab_append(ab, "\x1b[7m", 4);

  // Draw file name in status bar.
  char status[80],current_row_status[80];
  int len
    = snprintf(status, sizeof(status), "%.20s - %d lines, %s",
               E.filename ? E.filename : "[untitled]",
               E.num_rows,
               E.modified ? "(modified)" : "");

  int crs_len
    = snprintf(current_row_status, sizeof(current_row_status), "%d/%d",
               E.cursor_y + 1,
               E.num_rows);

  if (len > E.screen_cols)
    len = E.screen_cols;

  ab_append(ab, status, len);

  for(int i = len; i < E.screen_cols; i++)
    {
      if(E.screen_cols - i == crs_len)
        {
          ab_append(ab, current_row_status, crs_len);
          break;
        }
      else
        ab_append(ab, " ", 1);
    }

  ab_append(ab, "\x1b[m", 3);
  // Space for message bar.
  ab_append(ab, "\r\n", 2);
}

void editor_draw_message_bar(struct abuf *ab)
{
  ab_append(ab, "\x1b[K", 3);
  int msglen = strlen(E.status_msg);
  if (msglen > E.screen_cols)
    msglen = E.screen_cols;
  if (msglen && time(NULL) - E.status_msg_time < 5)
    ab_append(ab, E.status_msg, msglen);
}

void editor_refresh_screen()
{
  editorScroll();

  struct abuf ab = ABUF_INIT;

  // Hide the cursor while typing
  ab_append(&ab, "\x1b[?25l", 6);
  ab_append(&ab, "\x1b[H", 3);

  editor_draw_rows(&ab);
  editor_draw_status_bar(&ab);
  editor_draw_message_bar(&ab);

  char cursor_buff[32];
  int len
    = snprintf(cursor_buff, 32, "\x1b[%d;%dH",
               (E.cursor_y - E.row_offset) + 1,
               (E.renderer_x - E.col_offset) + 1);

  ab_append(&ab, cursor_buff, len);

  ab_append(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO,ab.b,ab.len);
  ab_free(&ab);
}

/* Set the status message that would be displyed in the message bar.  */
void editor_set_status_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.status_msg, sizeof(E.status_msg), fmt, ap);
  va_end(ap);
  E.status_msg_time = time(NULL);
}

/************************ input ***********************/

void editor_navigate_cursor(int key)
{
  e_row *row = (E.cursor_y >= E.num_rows) ? NULL : &E.row[E.cursor_y];

  // navigating via wasd
  switch (key)
    {
      case ARROW_LEFT:
        if(E.cursor_x != 0)
          E.cursor_x--;
        // left arrow at the end of line
        else if (E.cursor_y > 0)
          {
            E.cursor_y--;
            E.cursor_x = E.row[E.cursor_y].size;
          }
        break;

      case ARROW_RIGHT:
        if(row && E.cursor_x < row->size)
          E.cursor_x++;
        // right arrow on begining of line
        else if (row && E.cursor_x == row->size)
          {
            E.cursor_y++;
            E.cursor_x = 0;
          }
        break;

      case ARROW_UP:
        if(E.cursor_y != 0)
          E.cursor_y--;
        break;

      case ARROW_DOWN:
        if(E.cursor_y < E.num_rows)
          E.cursor_y++;
        break;
    }

  // Clip cursor at the end of lines
  row = (E.cursor_y >= E.num_rows) ? NULL : &E.row[E.cursor_y];
  int rowlen = row ? row->size : 0;
  if(E.cursor_x > rowlen)
    E.cursor_y = rowlen;
}

void editor_process_keypress()
{
  static int quit_attempts = 0;
  int c = editor_read_key();

  switch(c)
    {
      // "ctrl + q" to quit
      case CTRL_KEY('q'):
        if(E.modified && quit_attempts < 1)
        {
          editor_set_status_message("File contains unsaved changes,"
                                    "Press CTRL-Q again to confirm quit.");
          quit_attempts++;
          break;
        }
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;

      // "ctrl + s" to save the buffer to disk
      case CTRL_KEY('s'):
      {
        bool save_sucess = editor_save();

        if (save_sucess)
        {
          editor_set_status_message("save succesfull !");
          E.modified = 0;
        }
        // TODO: Give out more info about what went wrong while saving.
        else
          editor_set_status_message("Can't save !");
        break;
      }

      // Navigation keys
      case ARROW_LEFT:
      case ARROW_RIGHT:
      case ARROW_DOWN:
      case ARROW_UP:
        editor_navigate_cursor(c);
        break;

      // Newline
      case '\r':
        editor_insert_newline();
        break;

      // Deletion keys
      case DEL_KEY:
      case BACKSPACE:
      case CTRL_KEY('h'):
        editor_delete_char(c);
        break;

      // TODO: Handle more key combinations.

      default:
        // Ordinary key was pressed, in such case, render the character as it is.
        editor_insert_char(c);
    }
}

/************************** init *************************/
void init_editor()
{
  if(get_windows_size(&E.screen_rows,&E.screen_cols) == -1)
    die("get_windows_size");

  E.cursor_x = 0;
  E.cursor_y = 0;
  E.renderer_x = 0;
  E.num_rows = 0;
  E.row_offset = 0;
  E.col_offset = 0;
  E.row = NULL;
  E.filename = NULL;
  E.status_msg[0] = '\0';
  E.status_msg_time = 0;
  E.modified = 0;
  // Leave space for status bar.
  E.screen_rows -= 2;
}

/************************** main() function *************************/
int main(int argc, char *argv[])
{
  enable_raw_mode();
  init_editor();
  if (argc >= 2)
    editor_open(argv[1]);

  editor_set_status_message ("HELP: Ctrl-S = save | Ctrl-Q = quit");

  while (1)
    {
      editor_refresh_screen();
      editor_process_keypress();
    }

  return 0;
}
