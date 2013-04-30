#include "httplibs.h"

int handle_request(const struct mime *mime_tbl, const struct cgi *cgi_tbl, const char* path_prefix, const int sockfd) {
    char buf[BUFFER_SIZE], local_path[BUFFER_SIZE];
    char basic_request[3][BUFFER_SIZE], *content_type=0, *query=0;
    struct request req;
    memset(buf, 0, sizeof buf);
    memset(local_path, 0, sizeof local_path);
    memset(&req, 0, sizeof(struct request));

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

    req.type = type;
    req.uri = basic_request[PATH];
    req.local_path = local_path;
    req.query_string = query;

    fprintf(stderr, "%s %s\n", basic_request[METHOD], basic_request[PATH]);

    write_socket(STR_PROC, strlen(STR_PROC), sockfd);
    if(type==GET) {
        FILE *fp = fopen(local_path, "r");
        if(fp==0) {
            /* File doesn't exist */
            write_socket(RES_404, strlen(RES_404), sockfd);
            write_socket("\r\n", 2, sockfd);
        } else {
            write_socket(RES_200, strlen(RES_200), sockfd);
            if(handle_cgi(&req, cgi_tbl, sockfd)==0) {
                content_type = find_content_type(mime_tbl, determine_ext(req.local_path));
                write_socket(FLD_CONTENT_TYPE, strlen(FLD_CONTENT_TYPE), sockfd);
                write_socket(content_type, strlen(content_type), sockfd);
                write_socket("\r\n", 2, sockfd);
                write_socket("\r\n", 2, sockfd);
                send_file(local_path, sockfd);
            }
        }
        fclose(fp);
    } else if(type==POST) {
        if(handle_cgi(&req, cgi_tbl, sockfd)==0) {
            return -1;
        }
    } else {
        write_socket(RES_400, strlen(RES_400), sockfd);
        write_socket("\r\n", 2, sockfd);
        write_socket("\r\n", 2, sockfd);
    }
    return 0;
}

int handle_cgi(const struct request *req, const struct cgi *cgi, const int sockfd) {
    char *cmd;
    char buf[BUFFER_SIZE];
    int cp[2], fork_stat;

    if(cgi==0) return 0;

    cmd = find_cgi_command(cgi, determine_ext(req->local_path));
    if(cmd==0) return 0;

    build_cgi_env(req);

    if(req->type==GET) {
        if(req->query_string) {
            setenv("CONTENT_LENGTH", "", 1);
            setenv("QUERY_STRING", req->query_string, 1);
        }

        if(pipe(cp)<0) {
            fprintf(stderr, "Cannot pipe\n");
            return -1;
        }

        pid_t pid;
        if((pid = vfork()) == -1) {
            fprintf(stderr, "Failed to fork new process\n");
            return -1;
        }

        if(pid==0) {
            close(sockfd);
            close(cp[0]);
            dup2(cp[1], STDOUT_FILENO);
            execlp(cmd, cmd, req->local_path, (char*) 0);
            exit(0);
        } else {
            close(cp[1]);
            int len;
            while((len=read(cp[0], buf, BUFFER_SIZE))>0) {
                buf[len] = '\0';
                str_strip(buf);
                len = strlen(buf);
                write_socket(buf, len, sockfd);
            }
            close(cp[0]);
            waitpid((pid_t)pid, &fork_stat, 0);
        }
    } else if(req->type==POST) {
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
        if((pid = vfork()) == -1) {
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
            execlp(cmd, cmd, req->local_path, (char*) 0);
            exit(0);
        } else {
            close(post_pipe[0]);
            close(cp[1]);
            write(post_pipe[1], buf, strlen(buf)+1);
            close(post_pipe[1]);
            memset(buf, 0, sizeof buf);
            int len;
            while((len=read(cp[0], buf, BUFFER_SIZE))>0) {
                buf[len] = '\0';
                str_strip(buf);
                len = strlen(buf);
                write_socket(buf, len, sockfd);
            }
            waitpid((pid_t)pid, &fork_stat, 0);
            close(cp[0]);
            write_socket("\r\n", 2, sockfd);
            write_socket("\r\n", 2, sockfd);
        }

    }

    return 1;
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
    struct mime *ptr = 0, *root = 0;
    FILE *fp = fopen("mime.types","r");
    int i;

    memset(buf, 0, sizeof buf);

    if(fp==0) {
        fprintf(stderr, "Cannot open mime.types\n");
        fclose(fp);
        return 0;
    }

    /* Initialize two-level structure */
    root = (struct mime*) malloc(sizeof(struct mime));
    memset(root, 0, sizeof(struct mime));
    root->next_level = (struct mime*) malloc(sizeof(struct mime) * 26);
    memset(root->next_level, 0, sizeof(struct mime) * 26);
    for(i=0;i<26;i++) {
        root->next_level[i].next_level = (struct mime*) malloc(sizeof(struct mime) * 26);
        memset(root->next_level[i].next_level, 0, sizeof(struct mime) * 26);
    }

    while(fgets(buf, BUFFER_SIZE, fp)!=0) {
        sscanf(buf, "%s %s", ext, type);

        /* Create new node in advance */
        ptr = (struct mime*) malloc(sizeof(struct mime));
        memset(ptr, 0, sizeof(struct mime));
        ptr->ext = (char*) malloc(sizeof(char) * (strlen(ext)+1));
        ptr->type = (char*) malloc(sizeof(char) * (strlen(type)+1));
        strncat(ptr->ext, ext, strlen(ext));
        strncat(ptr->type, type, strlen(type));

        struct mime *first_level = &(root->next_level[ext[0]-'a']);
        struct mime *second_level = &(first_level->next_level[ext[1]-'a']);

        if(strlen(ext)==1) {
            if(first_level->ext==0) {
                /* Initial first node of level 1 */
                first_level->ext = (char*) malloc(sizeof(char) * (strlen(ext)+1));
                memset(first_level->ext, 0, sizeof(char) * (strlen(ext)+1));
                first_level->type = (char*) malloc(sizeof(char) * (strlen(type)+1));
                memset(first_level->type, 0, sizeof(char) * (strlen(type)+1));
                strncat(first_level->ext, ext, strlen(ext));
                strncat(first_level->type, type, strlen(type));
                free(ptr);
            } else {
                /* Append node to level 1 */
                /* Set first_level to the last node and append previously created ptr to it*/
                while(first_level->next!=0) first_level = first_level->next;
                first_level->next = ptr;
            }
        } else {
            if(second_level->ext==0) {
                /* Initial first node of level 2 */
                second_level->ext = (char*) malloc(sizeof(char) * (strlen(ext)+1));
                memset(second_level->ext, 0, sizeof(char) * (strlen(ext)+1));
                second_level->type = (char*) malloc(sizeof(char) * (strlen(type)+1));
                memset(second_level->type, 0, sizeof(char) * (strlen(type)+1));
                strncat(second_level->ext, ext, strlen(ext));
                strncat(second_level->type, type, strlen(type));
                free(ptr);
            } else {
                /* Append node to level 1 */
                /* Set second_level to the last node and append previously created ptr to it*/
                while(second_level->next!=0) second_level = second_level->next;
                second_level->next = ptr;
            }
        }
        memset(buf, 0, sizeof buf);
    }

    fclose(fp);
    return root;
}

char * find_content_type(const struct mime *tbl, const char *ext) {
    struct mime *first_level = &(tbl->next_level[ext[0]-'a']);
    struct mime *second_level = &(first_level->next_level[ext[1]-'a']);
    struct mime *start = (strlen(ext)==1) ? first_level : second_level;

    while(start) {
        if(strcmp(ext, start->ext)==0) {
            return start->type;
        }
        start = start->next;
    }
    return 0;
}

char * find_cgi_command(const struct cgi *tbl, const char *ext) {
    while(tbl!=0) {
        if(strcmp(tbl->ext,ext)==0) return tbl->cmd;
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

int build_cgi_env(const struct request *req) {
    const char* local_path = req->local_path;
    const char* uri = req->uri;
    const int req_type = req->type;

    setenv("SERVER_NAME", "localhost", 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("REQUEST_URI", uri, 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("REDIRECT_STATUS", "200", 1);
    setenv("SCRIPT_FILENAME", local_path, 1);
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
    while(*ptr!=0) ptr++;
    --ptr;
    while((isspace(*ptr))) {
        *ptr = 0;
        ptr--;
    }
    return str;
}

struct cgi * init_cgi_table() {
    char buf[BUFFER_SIZE], key[BUFFER_SIZE], val[BUFFER_SIZE];
    struct cgi *ptr = 0, *root = 0;
    FILE *fp = fopen("cgi.conf","r");

    memset(buf, 0, sizeof buf);

    if(fp==0) {
        fprintf(stderr, "Cannot open cgi.conf\n");
        fclose(fp);
        return 0;
    }

    while(fgets(buf, BUFFER_SIZE, fp)!=0) {
        str_strip(buf);
        if(strcmp("[CGI]", buf)!=0) {
            return 0;
        }
        int i;
        if(ptr==0) {
            ptr = (struct cgi*) malloc(sizeof(struct cgi));
            root = ptr;
        } else {
            ptr->next = (struct cgi*) malloc(sizeof(struct cgi));
            ptr = ptr->next;
        }
        ptr->next = 0;
        for(i=0;i<2;i++) {
            if(fgets(buf, BUFFER_SIZE, fp)==0) {
                free(ptr);
                return 0;
            }
            sscanf(buf, "%s %s", key, val);

            if(strcmp("EXTNAME", key)==0) {
                ptr->ext = (char*) malloc(sizeof(char) * (strlen(val)+1));
                memset(ptr->ext, 0, sizeof(char) * (strlen(val)+1));
                strncat(ptr->ext, val, strlen(val));
            } else if(strcmp("CMD", key)==0) {
                ptr->cmd = (char*) malloc(sizeof(char) * (strlen(val)+1));
                memset(ptr->cmd, 0, sizeof(char) * (strlen(val)+1));
                strncat(ptr->cmd, val, strlen(val));
            }
            memset(buf, 0, sizeof buf);
        }
    }

    fclose(fp);

    return root;    
}