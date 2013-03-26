#ifndef NETLIBS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int init_info(char* port, struct addrinfo** serv);
#endif