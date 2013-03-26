#include "netlibs.h"

int init_info(char* port, struct addrinfo** serv) {
    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status=getaddrinfo(0, port, &hints, serv)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return status;
    }
    return 0;
} 

int init_sock(struct addrinfo *info) {
    int sockfd;

    sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(sockfd==-1) {
        fprintf(stderr, "Fail to open socket\n");
        return -1;
    }

    if(bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
        fprintf(stderr, "Fail to bind port: %s\n", strerror(errno));
        return -1;
    }

    if(listen(sockfd, BACKLOG_SIZE) == -1) {
        fprintf(stderr, "Fail to listen on port: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
}