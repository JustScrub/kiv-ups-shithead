CC=gcc
OUT=server

all: $(OUT)

$(OUT): 
	$(CC) -Iinclude *.c -lpthread  -o $(OUT) -Wall -pedantic

clean: 
	rm $(OUT)

rebuild: clean all