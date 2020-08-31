/****************** headers *************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

// trying to mirror what the control key does in terminal using bit masking
#define CTRL_KEY(k) ((k) & 0x1f)

/***************** global variables **********************/
struct termios orig_termios;

/***************** error handling ************************/
void die(const char *s)
{
  perror(s);
  exit(1);
}

/***************** terminal *****************************/
void disable_raw_mode ()
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)		// resetting the terminal back to normal
	{
		die("tcsetattr");
	}
}

void enable_raw_mode ()
{
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	atexit(disable_raw_mode);		// diables raw mode at exit of the program

	struct termios raw = orig_termios;		// to make sure that the changes don't take place in global variable
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

/************************ input ***********************/
void editor_process_keypress()
{
    char c = editor_read_key();
    if(iscntrl(c))
        {
            printf("%d\r\n",c);
        }
    else
        {
            printf("%d ('%c')\r\n", c, c);
        }

    if (c == CTRL_KEY('q')) exit(0);     // Ctrl + q to exit safely 👍
}

/************************** main() function *************************/
int main()
{
	enable_raw_mode();	

    while (1)
    {
        editor_process_keypress();
    }
	return 0;
}
