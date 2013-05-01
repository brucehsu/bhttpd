#include "bhttpd.h"

int main(int argc, char **argv) {
    const int POLL_MAX = sysconf(_SC_OPEN_MAX);
    struct pollfd fds[POLL_MAX];
    int sockfd, clifd, polled=0, i;
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

    memset(fds, 0, sizeof(struct pollfd)*POLL_MAX);
    for(i=0;i<POLL_MAX;i++) fds[i].fd = -1;
    fds[0].fd = sockfd;
    fds[0].events = POLLRDNORM;


    while(1) {
        polled = poll(fds, POLL_MAX, -1);
        if(polled==-1) {
            fprintf(stderr, "poll() error\n");
            continue;
        }

        if(fds[0].revents & POLLRDNORM) {
            /* Handle new connection */
            clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &addr_size);
            for(i=1;i<POLL_MAX;i++) {
                if(fds[i].fd==-1) {
                    fds[i].fd = clifd;
                    fds[i].events = POLLRDNORM;
                    break;
                }
            }
        }

        for(i=1;i<POLL_MAX;i++) {
            if(fds[i].fd!=-1&&(fds[i].revents & POLLRDNORM)) {
                handle_request(mime_tbl, cgi_tbl, conf.pub_dir, conf.default_page, fds[i].fd);
                close(fds[i].fd);
                fds[i].fd = -1;
                --polled;
            }
            if(polled==0) break;
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
        } else if(strcmp("DEFAULT_PAGE", param_name)==0) {
            conf->default_page = (char*) malloc(sizeof(char)*val_len);
            memset(conf->default_page, 0, sizeof(char)*val_len);
            strncpy(conf->default_page, param_val, val_len);
        }

        memset(buf, 0, sizeof buf);
        memset(param_name, 0, sizeof param_name);
        memset(param_val, 0, sizeof param_val);
    }
    return 0;
}
