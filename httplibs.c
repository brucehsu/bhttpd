#include "httplibs.h"

int handle_request(const struct mime *mime_tbl, const char* path_prefix, const int sockfd) {
    char buf[BUFFER_SIZE], local_path[BUFFER_SIZE];
    char basic_request[3][BUFFER_SIZE], *content_type;
    memset(buf, 0, sizeof buf);
    memset(local_path, 0, sizeof local_path);

    read_line(buf, sockfd);
    sscanf(buf, "%s %s %s", basic_request[METHOD], basic_request[PATH], basic_request[PROC]);

    const int type = parse_request_type(basic_request[METHOD]);
    strncat(local_path, path_prefix, BUFFER_SIZE-1);
    strncat(local_path, basic_request[PATH], BUFFER_SIZE-1);

    write_socket(STR_PROC, strlen(STR_PROC), sockfd);
    if(type==GET) {
        FILE *fp = fopen(local_path, "r");
        if(fp==0) {
            /* File doesn't exist */
            if(strlen(basic_request[PATH])==1&&basic_request[PATH][0]=='/') {
                /* TODO: Send default page */
            } else {
                write_socket(RES_404, strlen(RES_404), sockfd);
                write_socket("\r\n", 2, sockfd);
                write_socket("\r\n", 2, sockfd);
            }
        } else {
            write_socket(RES_200, strlen(RES_200), sockfd);
            write_socket(FLD_CONTENT_TYPE, strlen(FLD_CONTENT_TYPE), sockfd);
            content_type = find_content_type(mime_tbl, determine_ext(local_path));
            write_socket(content_type, strlen(content_type), sockfd);
            write_socket("\r\n", 2, sockfd);
            write_socket("\r\n", 2, sockfd);
            send_file(local_path, sockfd);
        }
        fclose(fp);
    } else if(type==POST) {
        /* TODO: POST action */
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