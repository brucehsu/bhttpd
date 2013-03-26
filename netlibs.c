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