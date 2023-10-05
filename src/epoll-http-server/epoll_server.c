#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "epoll_server.h"

#define MAXSIZE 2000

const char *get_file_type(const char *name)
{
    char* dot;

    dot = strrchr(name, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

int init_listen_fd(int port, int epfd)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1) {  
        perror("socket error");
        exit(1);
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    int flag = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
    int ret = bind(lfd, (struct sockaddr*)&serv, sizeof(serv));
    if(ret == -1) {    
        perror("bind error");
        exit(1);
    }

    ret = listen(lfd, 64);
    if(ret == -1) {    
        perror("listen error");
        exit(1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if(ret == -1) {  
        perror("epoll_ctl add lfd error");
        exit(1);
    }
    return lfd;
}

void do_accept(int lfd, int epfd)
{
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int cfd = accept(lfd, (struct sockaddr*)&client, &len);
    if (cfd == -1) {
        perror("accept error");
        exit(1);
    }

    char ip[64] = {0};
    printf("New client IP: %s, Port: %d, cfd = %d\n",
            inet_ntop(AF_INET, &client.sin_addr.s_addr, ip, sizeof(ip)),
            ntohs(client.sin_port), cfd);
    
    fcntl(cfd, F_SETFL, O_NONBLOCK);

    struct epoll_event ev;
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET; 
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1) {
        perror("epoll_ctl add cfd error");
        exit(1);
    }
}

int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                if (n > 0 && c == '\n') {
                    recv(sock, &c, 1, 0);
                } else {
                    c = '\n';
                }
            }
            buf[i] = c;
            ++i;
        } else {
            c = '\n';
        }
    }
    buf[i] = '\0';
    return i;
    
}

void disconnect(int cfd, int epfd)
{
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    if (ret == -1) {
        perror("epoll_ctl del cfd error");
        exit(1);
    }
    close(cfd);
}

int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

void encode_str(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {    
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {      
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

void decode_str(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from  ) {     
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {       
            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;                      
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}

void send_error(int cfd, int status, char *title, char *text)
{
	char buf[4096] = {0};

	sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
	sprintf(buf + strlen(buf), "Content-Type:%s\r\n", "text/html");
	sprintf(buf + strlen(buf), "Content-Length:%d\r\n", -1);
	sprintf(buf + strlen(buf), "Connection: close\r\n");
	send(cfd, buf, strlen(buf), 0);
	send(cfd, "\r\n", 2, 0);

	memset(buf, 0, sizeof(buf));

	sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
	sprintf(buf+strlen(buf), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
	sprintf(buf+strlen(buf), "%s\n", text);
	sprintf(buf+strlen(buf), "<hr>\n</body>\n</html>\n");
	send(cfd, buf, strlen(buf), 0);
	
	return ;
}

void send_respond_head(int cfd, int no, const char* desp, const char* type, long len)
{
    char buf[1024] = {0};

    sprintf(buf, "Http/1.1 %d %s\r\n", no, desp);
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", type);
    sprintf(buf + strlen(buf), "Content-Length:%d\r\n", -1);
	sprintf(buf + strlen(buf), "Connection: close\r\n");
	send(cfd, buf, strlen(buf), 0);
	send(cfd, "\r\n", 2, 0);
}

void send_dir(int cfd, const char* dirname)
{
    int i, ret;

    char buf[4094] = {0};

    sprintf(buf, "<html><head><title>directory name: %s</title></head>", dirname);
    sprintf(buf+strlen(buf), "<body><h1>current directory: %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};
    
    struct dirent** ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);
    
    for(i = 0; i < num; ++i) {
    
        char* name = ptr[i]->d_name;

        sprintf(path, "%s/%s", dirname, name);
        printf("path = %s ===================\n", path);
        struct stat st;
        stat(path, &st);

        encode_str(enstr, sizeof(enstr), name);
        
        if(S_ISREG(st.st_mode)) {       
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        } else if(S_ISDIR(st.st_mode)) {    
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        }
        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("send error:");
                continue;
            } else {
                perror("send error:");
                exit(1);
            }
        }
        memset(buf, 0, sizeof(buf));
    }

    sprintf(buf+strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);

    printf("dir message send OK!!!!\n");
}

void send_file(int cfd, const char* filename)
{
    int fd = open(filename, O_RDONLY);
    if(fd == -1) {   
        send_error(cfd, 404, "Not Found", "NO such file or direntry");
        exit(1);
    }

    char buf[4096] = {0};
    int len = 0, ret = 0;
    while( (len = read(fd, buf, sizeof(buf))) > 0 ) {   
        ret = send(cfd, buf, len, 0);
        if (ret == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("send error:");
                continue;
            } else {
                perror("send error:");
                exit(1);
            }
        }
    }
    if(len == -1)  {  
        perror("read file error");
        exit(1);
    }

    close(fd);
}

void http_request(const char* request, int cfd) 
{
    char method[12], path[1024], protocol[12];
    sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);

    decode_str(path, path);
    char *file = path + 1;

    if (strcmp(path, "/") == 0) {
        file = "./";
    }

    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1) {
        send_error(cfd, 404, "Not Found", "NO such file or direntry");     
        return;
    }

    if(S_ISDIR(st.st_mode)) { 
        send_respond_head(cfd, 200, "OK", get_file_type(".html"), -1);
        send_dir(cfd, file);
    } else if(S_ISREG(st.st_mode)) {    
        send_respond_head(cfd, 200, "OK", get_file_type(file), st.st_size);
        send_file(cfd, file);
    }
}

void do_read(int cfd, int epfd)
{
    char line[1024] = {0};
    int len = get_line(cfd, line, sizeof(line));
    if (len == 0) {
        printf("Client disconnected...\n");
        disconnect(cfd, epfd);
    } else {
        printf("====head====\n");
        printf("data: %s", line);
        fflush(stdout);
        while(1) {
            char buf[1024] = {0};
            len = get_line(cfd, buf, sizeof(buf));
            if (buf[0] == '\n') {
                break;
            } else if (len == -1)
                break;
        }
        printf("====end====\n");
    }

    if (strncasecmp("get", line, 3) == 0) {
        http_request(line, cfd);

        disconnect(cfd, epfd);
    }
}

void epoll_run(int port)
{
    int i = 0;

    int epfd = epoll_create(MAXSIZE);
    if(epfd == -1) {   
        perror("epoll_create error");
        exit(1);
    }

    int lfd = init_listen_fd(port, epfd);

    struct epoll_event all[MAXSIZE];
    while(1) {
        int ret = epoll_wait(epfd, all, MAXSIZE, 0);
        if(ret == -1) {
        
            perror("epoll_wait error");
            exit(1);
        }

        for(i=0; i<ret; ++i)
        {
            struct epoll_event *pev = &all[i];
            if(!(pev->events & EPOLLIN)) {
                continue;
            }
            if(pev->data.fd == lfd){
                printf("======================before do_accept, ret = %d\n", ret);
                do_accept(lfd, epfd);
                printf("=========================================after do_accept\n");
            } else {
                printf("======================before do_read, ret = %d\n", ret);
                do_read(pev->data.fd, epfd);
                printf("=========================================after do_read\n");
            }
        }
    }
}