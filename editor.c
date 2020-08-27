#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

struct termios orig_termios;

void disableRawMode ()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);		// resetting the terminal back to normal
}

void enableRawMode ()
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);		// diables raw mode at exit of the program

	struct termios raw = orig_termios;		// to make sure that the changes don't take place in global variable
	raw.c_lflag &= ~( ECHO | ICANON | ISIG | IEXTEN);		// togalling canonical mode off and also tacking the problem with SIGCONT and SIGSTP
	raw.c_iflag &= ~(ICRNL | IXON);		// stoping XON\OFF to pause the transmission also ICRNL for ctrl-m
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);		// turning off all rest of the flags
	raw.c_oflag &= ~(OPOST);		//terminates all output processing 
	raw.c_oflag |= ~(CS8);		// to set the charater size to 8 bit per byte

	raw.c_cc[VMIN] = 0;		// to set up timeout for input
  	raw.c_cc[VTIME] = 1;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);	// turning on raw mode
}

int main()
{
	enableRawMode();	

    while (1)
    {
    	char c = '\0';

    	read(STDIN_FILENO, &c, 1);
    	if(iscntrl(c))
    	{
    		printf("%d\r\n",c);
    	}
    	else
    	{
    		printf("%d ('%c')\r\n", c, c);
    	}

    	if(c == 'q')
    		break;
    }
	return 0;
}
