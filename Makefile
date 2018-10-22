all: server client

test:
	gcc src/test.c -std=c99 -o bin/test

server: src/server.c src/helper.c include/helper.h
	gcc src/server.c src/helper.c -Iinclude -std=c99 -o bin/server -lpthread -g

client: src/client.c
	gcc src/client.c -o bin/client -g
	
clean:
	rm bin/*

spawn:
	bin/client 127.0.0.1:12079 wonderland.txt &
	bin/client 127.0.0.1:12079 shake.txt &
	bin/client 127.0.0.1:12079 lorem.txt &
	bin/client 127.0.0.1:12079 shake.txt &
	bin/client 127.0.0.1:12079 lorem.txt