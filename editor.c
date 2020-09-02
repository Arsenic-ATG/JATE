/****************** headers *************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

// trying to mirror what the control key does in terminal using bit masking
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
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)		// resetting the terminal back to normal
	{
		die("tcsetattr");
	}
}

void enable_raw_mode ()
{
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disable_raw_mode);		// diables raw mode at exit of the program

	struct termios raw = E.orig_termios;		// to make sure that the changes don't take place in global variable
	raw.c_lflag &= ~( ECHO | ICANON | ISIG | IEXTEN);		// togalling canonical mode off and also tacking the problem with SIGCONT and SIGSTP
	raw.c_iflag &= ~(ICRNL | IXON);		// stoping XON\OFF to pause the transmission also ICRNL for ctrl-m
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);		// turning off all rest of the flags
	raw.c_oflag &= ~(OPOST);		//terminates all output processing 
	raw.c_oflag |= ~(CS8);		// to set the charater size to 8 bit per byte

	raw.c_cc[VMIN] = 0;		// to set up timeout for input
  	raw.c_cc[VTIME] = 1;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcgetattr");	// turning on raw mode
}

char editor_read_key()
{
    char c;
    int charaters_read;
    while((charaters_read = read(STDIN_FILENO, &c, 1)) != 1)
    {
    	if( charaters_read == -1) die("read");
    }
    return c;
}

int get_windows_size(int *rows,int *cols)
{
	struct winsize ws;

	if(ioctl(STDOUT_FILENO , TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    		{
    			editor_read_key();
				return -1;
			}
	}
	else		// returns the current screen size of the terminal window
	{
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 0;
	}
}

void init_editor()
{
	if(get_windows_size(&E.screen_rows,&E.screen_cols)== -1) die("get_windows_size");
}

/************************ input ***********************/
void editor_process_keypress()
{
    char c = editor_read_key();
    
    // if(iscntrl(c))
    //     {
    //         printf("%d\r\n",c);
    //     }
    // else
    //     {
    //         printf("%d ('%c')\r\n", c, c);
    //     }
    
    if (c == CTRL_KEY('q')) 	// Ctrl/Cmd + q to exit safely 👍
    {
    	write(STDOUT_FILENO, "\x1b[2J", 4);
  		write(STDOUT_FILENO, "\x1b[H", 3);
    	exit(0);     
	}
}
/************************* output ****************************/
void editor_draw_rows()
{
	int y;
	for (y = 0; y < E.screen_rows; y++) 
	{
    	write(STDOUT_FILENO, "~\r\n", 3);
  	}
}

void editor_refresh_screen()
{
	write(STDOUT_FILENO,"\x1b[2J",4);

	editor_draw_rows();
	
	write(STDOUT_FILENO, "\x1b[H", 3);
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
