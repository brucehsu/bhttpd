#ifndef BHTTTPD_H
#define BUFFER_SIZE 8192
#include <stdio.h>
#include <stdlib.h>
#include "netlibs.h"
#include "httplibs.h"

struct serv_conf {
    char* port;
    char* pub_dir;
};

int init_conf(struct serv_conf* conf);

#endif