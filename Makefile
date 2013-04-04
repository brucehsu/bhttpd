CC=gcc
OPTS=-o bhttpd
OBJ=bhttpd.c netlibs.c httplibs.c

all:
	$(CC) $(OBJ) $(OPTS)
clean:
	rm bhttpd