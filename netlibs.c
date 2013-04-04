#include "netlibs.h"

int init_info(const char* port, struct addrinfo** serv) {
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

int init_sock(const struct addrinfo *info) {
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

int send_file(const char *file_path, const int sockfd) {
    int len=0, read=0;
    char rbuf[BUFFER_SIZE];
    FILE *fp = 0;

    memset(rbuf, 0, sizeof rbuf);
    if ((fp=fopen(file_path,"r"))==0){
        fprintf(stderr, "Could not open file: %s\n", file_path);
        return -1;
    }

    while(!feof(fp) && (read=fread(rbuf, 1, BUFFER_SIZE, fp))) {
        write_socket(rbuf, read, sockfd);
        memset(rbuf, 0, sizeof rbuf);
        len+=read;
    }
    return len;
}

int read_line(char *buf, const int sockfd) {
    int stat = 0;
    while(1) {
        stat = read_socket(buf, 1, sockfd);
        if(stat==0) return stat;
        if(*buf=='\n') {
            *buf = 0;
            return stat;
        }
        ++buf;
    }
    return stat;
}

int write_socket(const char *buf, const int len, const int sockfd) {
    int sent = send(sockfd, buf, len, 0);
    if(sent==-1) {
        fprintf(stderr, "Send error\n");
    }
    return sent;
}

int read_socket(char *buf, int len, const int sockfd) {
    return recv(sockfd, buf, len, 0);
}