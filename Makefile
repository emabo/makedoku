CC=gcc
CFLAGS=-O3 -Wall
CFLAGS+=-DHAVE_OPENSSL
#CFLAGS+=-DDEBUG
LDFLAGS=-lssl
SOURCES=sudo.c resolve_iterative.c resolve_recursive.c resolve.c main.c
EXEC=makedoku

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
