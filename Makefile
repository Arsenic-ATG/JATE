TARGET: exe

FLAGS = -Wall -Wextra -pedantic -std=c99

function: src/func_source.c
	$(CC) -I src/headers/ -c src/func_source.c -o src/func_source.o $(FLAGS)

error: src/error_handling.c
	$(CC) -I src/headers/ -c src/error_handling.c -o src/error.o $(FLAGS)

main: src/main.c
	$(CC) -c src/main.c -o src/main.o $(FLAGS)

exe: main function error
	$(CC) src/func_source.o src/main.o src/error.o -o exe $(FLAGS)

debug: main function error
	$(CC) src/func_source.o src/main.o src/error.o -o exe -O0 -g $(FLAGS)

clean:
	rm src/func_source.o
	rm src/error.o
	rm src/main.o
	rm exe
