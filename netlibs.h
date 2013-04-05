#ifndef NETLIBS_H
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

int init_info(const char* port, struct addrinfo** serv);
int init_sock(const struct addrinfo *info);
int send_file(const char *file_path, const int sockfd);
int read_line(char *buf, const int sockfd);
int write_socket(const char *buf, const int len, const int sockfd);
int read_socket(char *buf, int len, const int sockfd);
#endif