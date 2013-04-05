#include "bhttpd.h"

static int worker_count = 0;

int main(int argc, char **argv) {
    int sockfd, clifd, fork_stat, request_count;
    pid_t pid;
    struct serv_conf conf;
    struct mime * mime_tbl;
    struct addrinfo *info;
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;

    init_conf(&conf);
    init_info(conf.port, &info);
    mime_tbl = init_mime_table();

    sockfd = init_sock(info);
    if(sockfd==-1) return -1;

    struct sigaction act;
    act.sa_handler = terminate_zombie;
    act.sa_flags = SA_NOCLDSTOP;
    sigaction( SIGCHLD, &act, 0);

    setenv("SERVER_PORT", conf.port, 1);

    addr_size = sizeof(cli_addr);
    while(1) {
        /* Create worker process */
        for(;worker_count<conf.workers;worker_count++) {
            if((pid = fork()) == -1) {
                fprintf(stderr, "Failed to fork new process\n");
                return -1;
            }
            if(pid==0) {
                break;
            }
        }

        if(pid==0) {
            /* Worker Process */
            for(request_count=0;request_count<conf.requests;request_count++) {
                clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &addr_size);
                if(cli_addr.ss_family==AF_INET) {
                    /* IPv4 */
                    struct sockaddr_in *cli = (struct sockaddr_in*) &cli_addr;
                    setenv("REMOTE_ADDR", inet_ntoa(cli->sin_addr), 1);
                } else {
                    /* IPv6 */
                }
                handle_request(mime_tbl, conf.pub_dir, clifd);
                close(clifd);
            }
            close(sockfd);
            exit(0);
        } else {
            /* Parent Process waits for any worker to exit */
            waitpid(-1,&fork_stat,0);
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

void terminate_zombie() {
    int pid;
    int stat;
    while((pid=waitpid(-1, &stat, WNOHANG))>0) {
        worker_count--;
    }
}
