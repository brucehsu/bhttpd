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
#include <ctype.h>
#include "netlibs.h"

struct mime {
    char *ext;
    char *type;
    struct mime *next;
    struct mime *next_level;
}; 

struct cgi {
    char *ext;
    char *cmd;
    struct cgi *next;
};

struct request {
    int type;
    char * uri;
    char * local_path;
    char * query_string;
};

int handle_request(const struct mime *mime_tbl, const struct cgi *cgi_tbl, const char *path_prefix, const char *default_page, const int sockfd);
int handle_cgi(const struct request *req, const struct cgi *cgi, const int sockfd);
int parse_request_type(const char *buf);
struct mime * init_mime_table();
char * find_content_type(const struct mime *tbl, const char *ext);
char * find_cgi_command(const struct cgi *tbl, const char *ext);
char * determine_ext(const char *path);
int build_cgi_env(const struct request *req);
char * has_parameter(const char *uri);
char * str_strip(char *str);
struct cgi * init_cgi_table();
#endif