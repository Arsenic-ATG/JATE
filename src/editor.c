/****************** headers *************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>


// attmpet to mirror what the ctrl key does in terminal using bit masking
#define CTRL_KEY(k) ((k) & 0x1f)

/***************** global variables **********************/
struct editor_config
{
	int screen_rows;
	int screen_cols;
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

/***************** terminal *****************************/
void disable_raw_mode ()
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
	{
		die("tcsetattr");
	}
}

void enable_raw_mode ()
{
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disable_raw_mode);

	// make sure that the changes don't take place in global variable
	struct termios raw = E.orig_termios;
	
	// togal canonical mode off and also tacking the problem with SIGCONT and SIGSTP
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  	raw.c_oflag &= ~(OPOST);
  	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	// to set up timeout for input
	raw.c_cc[VMIN] = 0;		
  	raw.c_cc[VTIME] = 1;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcgetattr");
}

char editor_read_key()
{
    char c;
    int charaters_read = read(STDIN_FILENO, &c, 1);
    while(charaters_read != 1)
    {
    	charaters_read = read(STDIN_FILENO, &c, 1);
    	if( charaters_read == -1) die("read");
    }
    return c;
}

int get_cursor_position(int *rows,int *cols)
{
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	while(i < sizeof(buf) - 1)
	{
		if (read(STDIN_FILENO,&buf[i],1) != 1) break;
		else if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	 
	return 0;
}

int get_windows_size(int *rows,int *cols)
{
	struct winsize ws;

	if(ioctl(STDOUT_FILENO , TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) 
			return -1;
    	
    	return get_cursor_position(rows,cols);
	}
	else		// return the current screen size of the terminal window
	{
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 0;
	}
}

/************************append buffer********************/

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

// using append buffer to paint to prevent flickring while typing
void editor_draw_rows(struct abuf *ab)
{
	int y;
	for (y = 0; y < E.screen_rows; y++) 
	{
    	ab_append(ab, "~", 1);
    	// clear in line
		ab_append(ab, "\x1b[K", 3);

	  	if (y < E.screen_rows - 1) 
	    	ab_append(ab, "\r\n", 2);
	}
}

// formatting requires improvement here
void editor_refresh_screen()
{
	struct abuf ab = ABUF_INIT;

	//hide the cursor while typing
	ab_append(&ab, "\x1b[?25l", 6);
	ab_append(&ab, "\x1b[H", 3);

	editor_draw_rows(&ab);

	ab_append(&ab, "\x1b[H", 3);
	ab_append(&ab, "\x1b[?25l", 6);

	write(STDOUT_FILENO,ab.b,ab.len);
	ab_free(&ab);
}

/************************ input ***********************/
void editor_process_keypress()
{
    char c = editor_read_key();
    
    switch(c)
    {
    	case CTRL_KEY('q'): write(STDOUT_FILENO, "\x1b[2J", 4);
  							write(STDOUT_FILENO, "\x1b[H", 3);
    						exit(0);
    						break;
    }
}

void init_editor()
{
	if(get_windows_size(&E.screen_rows,&E.screen_cols)== -1) die("get_windows_size");
}

/************************** main() function *************************/
int main()
{
	enable_raw_mode();
	init_editor();

    while (1)
    {
    	editor_refresh_screen();
        editor_process_keypress();
    }

	return 0;
}
