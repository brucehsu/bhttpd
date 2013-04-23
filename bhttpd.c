#include "bhttpd.h"

int main(int argc, char **argv) {
    fd_set allset, rset;
    int sockfd, clifd, maxfd, readycnt,i;
    struct serv_conf conf;
    struct mime * mime_tbl;
    struct cgi * cgi_tbl;
    struct addrinfo *info;
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;

    init_conf(&conf);
    init_info(conf.port, &info);
    mime_tbl = init_mime_table();
    cgi_tbl = init_cgi_table();

    sockfd = init_sock(info);
    if(sockfd==-1) return -1;

    FD_ZERO(&allset);
    FD_ZERO(&rset);
    FD_SET(sockfd, &allset);
    maxfd = sockfd;

    setenv("SERVER_PORT", conf.port, 1);

    addr_size = sizeof(cli_addr);

    while(1) {
        rset = allset;
        if((readycnt=select(maxfd+1, &rset, NULL, NULL, NULL)) == -1) {
            fprintf(stderr, "select error\n");
        }

        if(FD_ISSET(sockfd, &rset)) {
            /* Handle new connection */
            clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &addr_size);
            FD_SET(clifd, &allset);
            if(clifd>maxfd) maxfd = clifd;
        }

        for(i=0;i<=maxfd;i++) {
            if(i==sockfd) continue;

            if(FD_ISSET(i, &rset)) {
                handle_request(mime_tbl, cgi_tbl, conf.pub_dir, i);
                FD_CLR(i, &allset);
                close(i);
            }
        }
    }
    return 0;
}

int init_conf(struct serv_conf* conf) {
    FILE* fp;
    int val_len = 0;
    char buf[BUFFER_SIZE];
    char param_name[BUFFER_SIZE];
    char param_val[BUFFER_SIZE];

    memset(buf, 0, sizeof(buf));
    memset(param_name, 0, sizeof(param_name));
    memset(param_val, 0, sizeof(param_val));

    if ((fp=fopen("serv.conf","r"))==0){
        fprintf(stderr, "Could not read configuration file\n");
        return -1;
    }

    while(fgets(buf, BUFFER_SIZE, fp)!=0) {
        char *ptr = buf;
        char *to_ptr = param_name;
        while(*ptr!=' ') *to_ptr++ = *ptr++;
        while(*ptr==' ' || *ptr=='\t') ++ptr;
        to_ptr = param_val;
        while(*ptr!=0 && *ptr!='\n') *to_ptr++ = *ptr++;
        val_len = strlen(param_val) + 1;

        if(strcmp("PORT", param_name)==0) {
            conf->port = (char*) malloc(sizeof(char)*val_len);
            memset(conf->port, 0, sizeof(char)*val_len);
            strncpy(conf->port, param_val, val_len);
        } else if(strcmp("DIRECTORY", param_name)==0) {
            conf->pub_dir = (char*) malloc(sizeof(char)*val_len);
            memset(conf->pub_dir, 0, sizeof(char)*val_len);
            strncpy(conf->pub_dir, param_val, val_len);
        } else if(strcmp("WORKERS", param_name)==0) {
            conf->workers = atoi(param_val);
        } else if(strcmp("REQUEST_PER_WORKER", param_name)==0) {
            conf->requests = atoi(param_val);
        }

        memset(buf, 0, sizeof buf);
        memset(param_name, 0, sizeof param_name);
        memset(param_val, 0, sizeof param_val);
    }
    return 0;
}
