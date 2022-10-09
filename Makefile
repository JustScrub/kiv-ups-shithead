CC=gcc

all: main

main: 
	$(CC) -o main -pthread *.c

clean: 
	rm main