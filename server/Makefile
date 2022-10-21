CC=gcc
OUT=main

all: $(OUT)

main: 
	$(CC) -o $(OUT) -pthread *.c -pedantic
	chmod 711 $(OUT)

clean: 
	rm $(OUT)