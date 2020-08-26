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
	raw.c_lflag &= ~( ECHO | ICANON | ISIG);		// togalling canonical mode off and also tacking the problem with SIGCONT and SIGSTP
	raw.c_iflag &= ~(IXON);		// stoping XON\OFF to pause the transmission

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);	// turning on raw mode
}

int main()
{
	enableRawMode();	

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c!='q')
    {
    	if(iscntrl(c))
    	{
    		printf("%d\n",c);
    	}
    	else
    	{
    		printf("%d ('%c')\n", c, c);
    	}
    }
	return 0;
}
