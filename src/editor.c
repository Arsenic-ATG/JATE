/****************** headers *************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>

// attmpt to mirror what the ctrl key does in terminal using bit masking
#define CTRL_KEY(k) ((k) & 0x1f)

#define TAB_SIZE 4

/***************** global variables **********************/

enum key
{
	ARROW_UP = -1,
	ARROW_DOWN ,
	ARROW_LEFT ,
	ARROW_RIGHT
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
	char *filename;
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
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) 
		die("tcgetattr");

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

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) 
		die("tcgetattr");
}

char editor_read_key()
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
			switch (seq[1]) 
			{
				case 'A': return ARROW_UP;
				case 'B': return ARROW_DOWN;
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
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
	else		// return the current screen size of the terminal window
	{
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
	// find tabs allocate required amount of memory dynamically
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

void editor_append_row(char *s, size_t len) 
{
	E.row = realloc(E.row, sizeof(e_row) * (E.num_rows + 1));

	int at = E.num_rows;
	E.row[at].size = len;
	E.row[at].text = malloc(len + 1);
	memcpy(E.row[at].text, s, len);
	E.row[at].text[len] = '\0';

	E.row[at].r_size = 0;
	E.row[at].renderer = NULL;
	editor_update_row(&E.row[at]);

	E.num_rows++;
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
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
			linelen--;

		editor_append_row(line, linelen);

		linelen = getline(&line, &linecap, fp);
	}

	free(line);
	fclose(fp);
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
			// welcome mesage
			if (E.num_rows == 0 && y == E.screen_rows/8)
			{
				char welcome_buffer[60];
				int message_length = snprintf ( welcome_buffer, sizeof(welcome_buffer) , "Welcome the the Text Editor " );

				if (message_length > E.screen_cols)
					message_length = E.screen_cols;

				// subracting one for first ">" character
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
				// start of new line
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

    	// clear in line
		ab_append(ab, "\x1b[K", 3);

    	ab_append(ab, "\r\n", 2);
	}
}

void editor_draw_statusbar(struct abuf *ab)
{
	ab_append(ab, "\x1b[7m", 4);

	// draw file name in status bar
	char status[80],current_row_status[80];
	int len = snprintf(status, sizeof(status), "%.20s - %d lines", E.filename ? E.filename : "[untitled]", E.num_rows);

	int crs_len = snprintf(current_row_status, sizeof(current_row_status), "%d/%d", E.cursor_y + 1, E.num_rows);

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
}

// formatting requires improvement here
void editor_refresh_screen()
{
	editorScroll();

	struct abuf ab = ABUF_INIT;

	//hide the cursor while typing
	ab_append(&ab, "\x1b[?25l", 6);
	ab_append(&ab, "\x1b[H", 3);

	editor_draw_rows(&ab);
	editor_draw_statusbar(&ab);

	char cursor_buff[32];
	int len = snprintf(cursor_buff, 32, "\x1b[%d;%dH",  (E.cursor_y - E.row_offset) + 1, 
														(E.renderer_x - E.col_offset) + 1);

	ab_append(&ab, cursor_buff, len);

	ab_append(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO,ab.b,ab.len);
	ab_free(&ab);
}

/************************ input ***********************/

void editor_navigate_cursor(char key) 
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

	// clip cursor at the end of lines
	row = (E.cursor_y >= E.num_rows) ? NULL : &E.row[E.cursor_y];
	int rowlen = row ? row->size : 0;
	if(E.cursor_x > rowlen)
		E.cursor_y = rowlen;
}

void editor_process_keypress()
{
    char c = editor_read_key();
    
    switch(c)
    {
    	case CTRL_KEY('q'): 
    		write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_DOWN:
		case ARROW_UP:
			editor_navigate_cursor(c);
			break;

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
	// leave space for status bar
	E.screen_rows --;

}

/************************** main() function *************************/
int main(int argc, char *argv[])
{
	enable_raw_mode();
	init_editor();
	if (argc >= 2)
	    editor_open(argv[1]);

    while (1)
    {
    	editor_refresh_screen();
        editor_process_keypress();
    }

	return 0;
}
