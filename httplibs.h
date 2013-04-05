#ifndef HTTPLIBS_H
#define METHOD 0
#define PATH 1
#define PROC 2
#define GET 13
#define STR_PROC "HTTP/1.1 "
#define RES_200 "200 OK\n"
#define RES_400 "400 Bad Request\n"
#define RES_404 "404 Not Found\n"
#define FLD_CONTENT_TYPE "Content-Type: "
#define POST 14
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "netlibs.h"

struct mime {
    char *ext;
    char *type;
    struct mime *next;
}; 

int handle_request(const struct mime *mime_tbl, const char *path_prefix ,const int sockfd);
int parse_request_type(const char *buf);
struct mime * init_mime_table();
char * find_content_type(const struct mime *tbl, const char *ext);
char * determine_ext(const char *path);
int build_cgi_env(const char* local_path, const char *uri, const int req_type);
char * has_parameter(const char *uri);
#endif