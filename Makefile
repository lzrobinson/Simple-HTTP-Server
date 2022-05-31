server: server.c httplib.h
	gcc -Wall -g -o server server.c httplib.h -lpthread

all: server

valgrind:
	valgrind --leak-check=yes -s ./server 4 8000 project-2/

clean:
	rm -f *.o server