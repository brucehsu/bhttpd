#ifndef NETLIBS_H
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

int init_info(char* port, struct addrinfo** serv);
int init_sock(struct addrinfo *info);
int send_file(char *file_path, int sockfd);
int write_socket(char *buf, int len, int sockfd);
#endif