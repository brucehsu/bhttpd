#ifndef BHTTTPD_H
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include "netlibs.h"
#include "httplibs.h"

struct serv_conf {
    char* port;
    char* pub_dir;
    int workers;
    int requests;
};

int init_conf(struct serv_conf* conf);

#endif