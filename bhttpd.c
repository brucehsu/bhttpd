#include "bhttpd.h"

int main(int argc, char **argv) {
    int sockfd, clifd;
    pid_t pid;
    struct serv_conf conf;
    struct mime * mime_tbl;
    struct addrinfo *info;
    struct sockaddr_storage cli_addr;
    socklen_t addr_size;

    init_conf(&conf);
    init_info(conf.port, &info);

    sockfd = init_sock(info);
    if(sockfd==-1) return -1;
    mime_tbl = init_mime_table(); /* BUG: If use before init_sock, will cause failure */

    struct sigaction act;
    act.sa_handler = terminate_zombie;
    act.sa_flags = SA_NOCLDSTOP;
    sigaction( SIGCHLD, &act, 0);

    addr_size = sizeof(cli_addr);
    while(1) {
        clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &addr_size);
        char buf[BUFFER_SIZE];
        memset(buf,0,sizeof buf);

        /* TODO: Fork new process to handle request */
        /* TODO: Send local files to client */
        if((pid = fork()) == -1) {
            fprintf(stderr, "Failed to fork new process\n");
            return -1;
        }

        if(pid) {
            /* Parent process */
        } else {
            /* Child process */
            handle_request(mime_tbl, conf.pub_dir, clifd);
            close(sockfd);
            close(clifd);
        }
        if(pid==0) exit(0);
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

void terminate_zombie() {
    int pid;
    int stat;
    while((pid=waitpid(-1, &stat, WNOHANG))>0);
}