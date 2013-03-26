#include "bhttpd.h"

int main(int argc, char **argv) {
    int sockfd, clifd;
    struct serv_conf conf;
    struct addrinfo *info;
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;

    init_conf(&conf);
    init_info(conf.port, &info);

    sockfd = init_sock(info);
    if(sockfd==-1) return -1;

    addr_size = sizeof(cli_addr);
    while((clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &addr_size)) != -1) {
        /* TODO: Fork new process to handle request */
        /* TODO: Send local files to client */
        close(clifd);
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
    memset(buf, 0, sizeof(param_name));
    memset(buf, 0, sizeof(param_val));

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
        }

        memset(buf, 0, sizeof(buf));
        memset(buf, 0, sizeof(param_name));
        memset(buf, 0, sizeof(param_val));
    }
    return 0;
}