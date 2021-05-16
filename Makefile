CC=gcc
CC_FLAGS=-lpthread

SERVER_SOURCES=src/main.c src/net/server.c

server: build_dir
	$(CC) $(CC_FLAGS) $(SERVER_SOURCES) -o build/server 

build_dir:
	mkdir -p build

