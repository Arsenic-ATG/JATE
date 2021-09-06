editor:src/editor.c
	$(CC) src/editor.c -o editor -Wall -Wextra -pedantic -std=c99

dubug:src/editor.c
	$(CC) src/editor.c -o editor -O0 -g -Wall -Wextra -pedantic -std=c99

