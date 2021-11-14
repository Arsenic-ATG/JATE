#include "terminal.h"
#include "editor.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/***************** error handling ************************/

void
die (const char *s)
{
  write (STDOUT_FILENO, "\x1b[2J", 4);
  write (STDOUT_FILENO, "\x1b[H", 3);

  perror (s);
  exit (1);
}

/***************** terminal ************************/
void
disable_raw_mode ()
{
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die ("tcsetattr");
}

void
enable_raw_mode ()
{
  if (tcgetattr (STDIN_FILENO, &E.orig_termios) == -1)
    die ("tcgetattr");

  atexit (disable_raw_mode);

  // Make sure that the changes don't take place globally.
  struct termios raw = E.orig_termios;

  // Toggle canonical mode off and also problem tracking with SIGCONT and
  // SIGSTP.
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // To set up timeout for input.
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die ("tcgetattr");
}

int
get_cursor_position (int *rows, int *cols)
{
  char buf[32];
  unsigned int i = 0;

  if (write (STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while (i < sizeof (buf) - 1)
    {
      if (read (STDIN_FILENO, &buf[i], 1) != 1)
        break;
      else if (buf[i] == 'R')
        break;
      i++;
    }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf (&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int
get_windows_size (int *rows, int *cols)
{
  struct winsize ws;

  if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
      if (write (STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
        return -1;

      return get_cursor_position (rows, cols);
    }
  else
    {
      // Return the current screen size of the terminal window.
      *rows = ws.ws_row;
      *cols = ws.ws_col;
      return 0;
    }
}
