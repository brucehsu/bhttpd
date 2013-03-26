#ifndef NETLIBS_H
#define BACKLOG_SIZE 16
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

int init_info(char* port, struct addrinfo** serv);
int init_sock(struct addrinfo *info);
#endif