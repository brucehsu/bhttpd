#include "httplibs.h"

int handle_request(const struct mime *mime_tbl, const char* path_prefix, const int sockfd) {
    char buf[BUFFER_SIZE], local_path[BUFFER_SIZE];
    char basic_request[3][BUFFER_SIZE], *content_type, *query;
    memset(buf, 0, sizeof buf);
    memset(local_path, 0, sizeof local_path);

    int cp[2], fork_stat;

    read_line(buf, sockfd);
    sscanf(buf, "%s %s %s", basic_request[METHOD], basic_request[PATH], basic_request[PROC]);

    const int type = parse_request_type(basic_request[METHOD]);

    query = has_parameter(basic_request[PATH]);
    if(query) {
        *query = 0;
        ++query;
    }

    /* Add default page */
    if(strlen(basic_request[PATH])==1&&basic_request[PATH][0]=='/') {
        strcat(basic_request[PATH], "index.htm");
    }

    strncat(local_path, path_prefix, BUFFER_SIZE-1);
    strncat(local_path, basic_request[PATH], BUFFER_SIZE-1);

    build_cgi_env(local_path, basic_request[PATH], type);

    fprintf(stderr, "Worker %d: %s %s\n", getpid(), basic_request[METHOD], basic_request[PATH]);

    write_socket(STR_PROC, strlen(STR_PROC), sockfd);
    if(type==GET) {
        FILE *fp = fopen(local_path, "r");
        if(fp==0) {
            /* File doesn't exist */
            write_socket(RES_404, strlen(RES_404), sockfd);
            write_socket("\r\n", 2, sockfd);
        } else {
            write_socket(RES_200, strlen(RES_200), sockfd);
            content_type = find_content_type(mime_tbl, determine_ext(local_path));
            write_socket(FLD_CONTENT_TYPE, strlen(FLD_CONTENT_TYPE), sockfd);
            if(strcmp("php", determine_ext(local_path))==0) {
                write_socket("text/html", strlen("text/html"), sockfd);
                if(query) {
                    setenv("QUERY_STRING", query, 1);
                }

                if(pipe(cp)<0) {
                    fprintf(stderr, "Cannot pipe\n");
                    return -1;
                }

                pid_t pid;
                if((pid = fork()) == -1) {
                    fprintf(stderr, "Failed to fork new process\n");
                    return -1;
                }

                if(pid==0) {
                    close(sockfd);
                    close(cp[0]);
                    dup2(cp[1], STDOUT_FILENO);
                    execlp("php-cgi", "php-cgi", "-n", local_path, (char*) 0);
                    exit(0);
                } else {
                    close(cp[1]);
                    int len;
                    while((len=read(cp[0], buf, BUFFER_SIZE))>0) {
                        int i;
                        buf[len] = '\0';
                        for(i=0;i<len;i++) {
                            write_socket(buf+i, 1, sockfd);
                        }
                    }
                    waitpid((pid_t)pid, &fork_stat, 0);
                    write_socket("\r\n", 2, sockfd);
                    write_socket("\r\n", 2, sockfd);
                }
            } else {
                write_socket(content_type, strlen(content_type), sockfd);
                write_socket("\r\n", 2, sockfd);
                write_socket("\r\n", 2, sockfd);
                send_file(local_path, sockfd);
            }
        }
        fclose(fp);
    } else if(type==POST) {
        /* TODO: POST action */
        int post_pipe[2];
        char content_len[BUFFER_SIZE];
        write_socket(RES_200, strlen(RES_200), sockfd);
        memset(buf, 0, sizeof buf);

        while(read_line(buf, sockfd)) {
            if(buf[0]=='\r') break;
            char *ptr = buf;
            while(*ptr!=':') ++ptr;
            *ptr = 0;
            if(strcmp("Content-Type", buf)==0) {
                setenv("CONTENT_TYPE", str_strip(ptr+=2), 1);
            }
            memset(buf, 0, sizeof buf);
        }

        memset(buf, 0, sizeof buf);
        read_socket(buf, BUFFER_SIZE, sockfd);
        sprintf(content_len, "%d", (int) strlen(buf));
        setenv("CONTENT_LENGTH", content_len, 1);

        setenv("QUERY_STRING", buf, 1);

        if(pipe(cp)<0||pipe(post_pipe)<0) {
            fprintf(stderr, "Cannot pipe\n");
            return -1;
        }

        pid_t pid;
        if((pid = fork()) == -1) {
            fprintf(stderr, "Failed to fork new process\n");
            return -1;
        }

        if(pid==0) {
            close(sockfd);
            close(cp[0]);
            close(post_pipe[1]);
            dup2(post_pipe[0], STDIN_FILENO);
            close(post_pipe[0]);
            dup2(cp[1], STDOUT_FILENO);
            execlp("php-cgi", "php-cgi", local_path, (char*) 0);
            exit(0);
        } else {
            close(post_pipe[0]);
            close(cp[1]);
            write(post_pipe[1], buf, strlen(buf)+1);
            close(post_pipe[1]);
            memset(buf, 0, sizeof buf);
            int len;
            while((len=read(cp[0], buf, BUFFER_SIZE))>0) {
                int i;
                buf[len] = '\0';
                for(i=0;i<len;i++) {
                    write_socket(buf+i, 1, sockfd);
                }
            }
            waitpid((pid_t)pid, &fork_stat, 0);
            close(cp[0]);
            write_socket("\r\n", 2, sockfd);
            write_socket("\r\n", 2, sockfd);
        }

    } else {
        write_socket(RES_400, strlen(RES_400), sockfd);
        write_socket("\r\n", 2, sockfd);
        write_socket("\r\n", 2, sockfd);
    }
    return 0;
}

int parse_request_type(const char *buf) {
    if(strcmp(buf, "GET")==0) {
        return GET;
    } else if (strcmp(buf,"POST")==0) {
        return POST;
    }
    return 0;
}

struct mime * init_mime_table() {
    char buf[BUFFER_SIZE], ext[BUFFER_SIZE], type[BUFFER_SIZE];
    struct mime *ptr = 0, *init_ptr = 0;
    FILE *fp = fopen("mime.types","r");

    memset(buf, 0, sizeof buf);

    if(fp==0) {
        fprintf(stderr, "Cannot open mime.types\n");
        fclose(fp);
        return 0;
    }

    while(fgets(buf, BUFFER_SIZE, fp)!=0) {
        sscanf(buf, "%s %s", ext, type);

        if(ptr==0) {
            ptr = (struct mime*) malloc(sizeof(struct mime));
            init_ptr = ptr;
        } else {
            ptr->next = (struct mime*) malloc(sizeof(struct mime));
            ptr = ptr->next;
        }
        ptr->next = 0;
        ptr->ext = (char*) malloc(sizeof(char) * (strlen(ext)+1));
        memset(ptr->ext, 0, sizeof(char) * (strlen(ext)+1));
        strncat(ptr->ext, ext, strlen(ext));
        ptr->type = (char*) malloc(sizeof(char) * (strlen(type)+1));
        memset(ptr->type, 0, sizeof(char) * (strlen(type)+1));
        strncat(ptr->type, type, strlen(type));

        memset(buf, 0, sizeof buf);
    }

    fclose(fp);

    return init_ptr;
}

char * find_content_type(const struct mime *tbl, const char *ext) {
    while(tbl!=0) {
        if(strcmp(tbl->ext,ext)==0) return tbl->type;
        tbl = tbl->next;
    }
    return 0;
}

char * determine_ext(const char *path) {
    const int len = strlen(path);
    path += len-1;

    while(*path!='.') --path;
    return (char*)(path+1);
}

int build_cgi_env(const char* local_path, const char *uri, const int req_type) {
    char script_filename[BUFFER_SIZE];
    memset(script_filename, 0, sizeof script_filename);
    strcat(script_filename, getenv("PWD"));
    strcat(script_filename, "/");
    strcat(script_filename, local_path);

    setenv("SERVER_NAME", "localhost", 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("REQUEST_URI", uri, 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("REDIRECT_STATUS", "200", 1);
    setenv("SCRIPT_FILENAME", script_filename, 1);
    setenv("SCRIPT_NAME", uri, 1);

    if(req_type==GET) {
        setenv("REQUEST_METHOD", "GET", 1);
    } else if(req_type==POST) {
        setenv("REQUEST_METHOD", "POST", 1);
    }

    return 0;
}

char * has_parameter(const char *uri) {
    while(*uri!=0) {
        if(*uri=='?') return (char *) uri;
        ++uri;
    }

    return 0;
}

char * str_strip(char *str) {
    char *ptr = str;
    while(!(isspace(*ptr))) {
        ptr++;
    }
    *ptr=0;
    return str;
}