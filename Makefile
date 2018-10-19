all: server client

test:
	gcc src/test.c -std=c99 -o bin/test

server: src/server.c src/helper.c include/helper.h
	gcc src/server.c src/helper.c -Iinclude -std=c99 -o bin/server

client: src/client.c
	gcc src/client.c -o bin/client
	
clean:
	rm bin/*

spawn:
	bin/client 127.0.0.1:12079 wonderland.txt &
	bin/client 127.0.0.1:12079 wonderland.txt &
	bin/client 127.0.0.1:12079 wonderland.txt &
	bin/client 127.0.0.1:12079 wonderland.txt &
	bin/client 127.0.0.1:12079 wonderland.txt