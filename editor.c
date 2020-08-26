#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

struct termios orig_termios;

void disableRawMode ()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode ()
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);		// diables raw mode at exit of the program

	struct termios raw = orig_termios;		// to make sure that the changes don't take place in global variable
	raw.c_lflag &= ~( ECHO );
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);	// turning on raw mode
}

int main()
{
	enableRawMode();	

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c!='q');
	return 0;
}
