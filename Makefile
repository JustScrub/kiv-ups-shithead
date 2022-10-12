CC=gcc

all: main

main: 
	$(CC) -o main -pthread *.c -pedantic

clean: 
	rm main