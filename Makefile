CC=gcc
OPTS=-o bhttpd
OBJ=bhttpd.c netlibs.c

all:
	$(CC) $(OBJ) $(OPTS)
clean:
	rm bhttpd