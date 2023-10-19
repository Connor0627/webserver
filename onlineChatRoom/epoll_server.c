#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<pthread.h>
#include<sys/epoll.h>
#include<ctype.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define MAX_EVENTS 1024
#define PORT 8888

typedef void (*call_back)(int, void*); 

typedef struct user {
    char usr_id[8];
    char usr_name[256];
    char usr_key[40];
    int st;         // 0---offline 1---online
}user_msg;

typedef struct myevent_s       
{
    int fd;                     // 监听的文件描述符
    int events;                 // 对应监听的事件 EPOLLIN / EPOLLOUT
    call_back fun;              // 回调函数
    void *arg;                  // 上面回调函数的参3
    int status;                 // 是否在监听红黑树上, 1 --- 在, 0 --- 不在
    char buf[BUFSIZ];           // 读写缓冲区
    int len;                    // 本次从客户端读入缓冲区数据的长度
    long long last_active_time; // 该文件描述符最后在监听红黑树上的活跃时间
    user_msg um;                // 用户登陆的信息
    int log_step;               // 标记用户位于登陆的操作 0-- 未登陆  1 --- 输入账号  2 ---- 输入密码   3----- 成功登陆  4 --- 注册用户名  5 ------ 输入注册的密码   6 ------- 再次输入密码验证
}myevent_s;

user_msg Users[MAX_EVENTS];
int user_num;
int g_efd;
myevent_s g_events[MAX_EVENTS + 1];
char login_hint[] = "与服务器建立连接, 开始进行数据通信 ------ [OK]\n"
                    "          epoll服务器聊天室测试版          \n"
                    "    (1)匿名聊天   (2)登陆   (3) 注册        \n"
                    ">>> ";
int online_fd[MAX_EVENTS], l[MAX_EVENTS], r[MAX_EVENTS],\
        fd_pos[MAX_EVENTS], idx, online_num;

// 函数声明
void cb_read(int,  void*);                // 服务器端读事件
void cb_write(int,  void*);               // 向在线用户发送数据写事件
void logout(int cfd, void *);             // 用户登出事件
void login_menu(int cfd , void* arg);     // 登陆界面读事件
void login(int, void*);                   // 输入账号登陆 
void register_id(int, void*);             // 注册新账号
void get_uid(myevent_s*);                 // 获取一个未注册的UID
void load_usermsg();                      // 从文件加载已经注册过的用户信息
void close_cfd(int cfd, myevent_s *ev);
void event_set(myevent_s *ev, int fd, int events, call_back fun, void *arg);
void event_add(int epfd, myevent_s *ev);
void event_del(int epfd, myevent_s *ev);

void sys_error(const char *str) 
{
    perror(str);
    exit(1);
}

void list_init()
{
    r[0] = 1;
    l[1] = 0;
    idx = 2;
    for (int i = 3; i < MAX_EVENTS; ++i)
        fd_pos[i] = -1;
}

void list_push(int fd)
{
    fd_pos[fd] = idx;
    online_fd[idx] = fd;
    r[idx] = r[0];
    l[idx] =  0;
    l[r[0]] = idx;
    r[0] = idx;
    ++idx;
    ++online_num;
}

void list_del(int fd)
{
    int k = fd_pos[fd];
    l[r[k]] = l[k];
    r[l[k]] = r[k];
    online_num--;
}

void load_usermsg()
{
    FILE *fp = fopen("./user_msg","r");
    if (fp == NULL) 
        sys_error("load error");
    while (!feof(fp))
    {
        user_num++;
        fscanf(fp,"%s %s %s", Users[user_num].usr_id, Users[user_num].usr_name, Users[user_num].usr_key);  // 将文件的内容加载到保存用户信息的结构体数据中
        Users[user_num].st = 0;
    }
    user_num--;
}

void login_menu(int cfd, void *arg)
{
    myevent_s *ev = (myevent_s*)arg;
    char *buf = ev->buf;
    int ret = read(cfd, buf, BUFSIZ);
    if (ret <= 0) {
        close_cfd(cfd, ev);
        return;
    }
    if (buf[0] == '1') {
        sprintf(ev->um.usr_name, "Anonymous user %ld", time(NULL));
        strcpy(ev->um.usr_id, "00000");

        ev->log_step = 3;
        list_push(cfd);

        sprintf(buf,"<<<               用户: %s  已登录,当前在线人数为 %d          \n\n>>>", ev->um.usr_name, online_num);
        ev->len = strlen(buf);
        char s[] = "----------------------epoll聊天室测试版--------------------\n";
        write(cfd, s, sizeof(s));
        write(cfd, buf, ev->len);

        event_del(g_efd, ev);
        event_set(ev, cfd, EPOLLOUT , cb_write, ev);
        event_add(g_efd, ev);
    } else if (buf[0] == '2') {
        ev->log_step = 1;
        strcpy(buf, "请输入登陆的UID:");
        write(cfd, buf, strlen(buf));

        event_del(g_efd, ev);
        event_set(ev, cfd, EPOLLIN | EPOLLET , login, ev);
        event_add(g_efd, ev);
    } else {
        strcpy(buf, "注册账号\n###请输入注册的用户名(中文/英文, 注意不要包含特殊符号): ");
        write(cfd, buf, strlen(buf));
        ev->log_step = 4;

        event_del(g_efd, ev);
        event_set(ev, cfd, EPOLLIN | EPOLLET, register_id, ev);
        event_add(g_efd, ev);
    }
}

void login(int cfd, void *arg) {
    myevent_s *ev = (myevent_s*)arg;
    char *buf = ev->buf;
    int ret = read(cfd, buf, BUFSIZ);
    if (ret <= 0) {
        close_cfd(cfd, ev);
        return;
    }
    buf[ret - 1] = '\0';

    if (ev->log_step == 1)
    {
        int id = atoi(buf);
        strcpy(ev->um.usr_id, buf);
        char s[100];
        if (id > user_num || id <= 0) {
            sprintf(s, "!用户UID:%s 不存在\n请重新输入账号UID:", buf);
            write(cfd, s, strlen(s));
            return;
        }
        if (Users[id].st) {
            sprintf(s, "!用户UID:%s 已登陆\n请重新输入账号UID:", buf);
            write(cfd, s, strlen(s));
            return;
        }
        strcpy(buf, "请输入密码:");
        write(cfd, buf, strlen(buf));
    } else if (ev->log_step == 2) {
        int id = atoi(ev->um.usr_id);
        strcpy(ev->um.usr_key, buf);
        if (!strcmp(buf, Users[id].usr_key)) {
            strcpy(ev->um.usr_name, Users[id].usr_name);
            list_push(cfd);
            Users[id].st = 1;
            sprintf(buf,">               用户: %s  已登录,当前在线人数为 %d          \n\n>>>", ev->um.usr_name, online_num);
            ev->len = strlen(buf);
            char s[] = "----------------------epoll聊天室测试版--------------------\n";
            write(cfd, s, sizeof(s));
            write(cfd, buf, ev->len);

            event_del(g_efd, ev);
            event_set(ev, cfd, EPOLLOUT , cb_write, ev);
            event_add(g_efd, ev);
        } else {
            strcpy(buf, "密码错误, 请重新输入密码:");
            write(cfd, buf, strlen(buf));
            return;
        }
    }
    ev->log_step++;
}

void register_id(int cfd, void *arg) {
    myevent_s *ev = (myevent_s*)arg;
    char *buf = ev->buf;
    int ret = read(cfd, buf, BUFSIZ);
    if (ret <= 0) {
        close_cfd(cfd, ev);
        return;
    }
    buf[ret - 1] = '\0';
    if (ev->log_step == 4) {
        strcpy(ev->um.usr_name, buf);
        strcpy(buf, "请设定账号的密码: ");
        write(cfd, buf, strlen(buf));
    } else if (ev->log_step == 5) {
        strcpy(ev->um.usr_key, buf);
        strcpy(buf, "请再次输入密码: ");
        write(cfd, buf, strlen(buf));
    } else if (ev->log_step == 6) {
        if (strcmp(ev->um.usr_key, buf)) {
            if (strcmp(ev->um.usr_key, buf)) {
                strcpy(buf,"两次密码输入不一致, 请重新输入:");
                write(cfd, buf, strlen(buf));
                return;
            }
        }
        get_uid(ev);
        sprintf(buf, "注册成功, 你的账号uid: %s  用户名为%s, 现在重新返回登陆界面 \n\n", ev->um.usr_id,ev->um.usr_name);
        user_num++;
        strcpy(Users[user_num].usr_id, ev->um.usr_id);
        strcpy(Users[user_num].usr_name, ev->um.usr_name);
        strcpy(Users[user_num].usr_key, ev->um.usr_key);

        ev->log_step = 0;
        write(cfd, buf, strlen(buf));
        write(cfd, login_hint, sizeof login_hint);

        event_del(g_efd, ev);
        event_set(ev, cfd, EPOLLIN | EPOLLET, login_menu, ev);
        event_add(g_efd, ev);
        return;
    }
    ev->log_step++;
}

void logout(int cfd, void *arg) {
    myevent_s *ev = (myevent_s*)arg;
    char str[1024];
    list_del(cfd);
    ev->log_step = 0;
    Users[atoi(ev->um.usr_id)].st = 0;

    sprintf(str, "已退出聊天室, 当前在线人数为%d\n", online_num);
    sprintf(ev->buf, "(%s) %s\n>>>", ev->um.usr_name, str);
    ev->len = strlen(ev->buf);
    cb_write(cfd, ev);
}

void get_uid(myevent_s *ev)
{
    char str[10];
    user_msg *p = &ev->um;
    sprintf(str, "%05d", user_num + 1);
    strcpy(ev->um.usr_id, str);
    FILE *fp = fopen("./user_msg", "a+");
    if (fp == NULL) {
        write(ev->fd, "error\n", 6);
        fprintf(stderr, "get_uid open file error\n");
    }
    fprintf(fp, "%s %s %s\n", p->usr_id, p->usr_name, p->usr_key);
    fflush(fp);
}

// set listen event
void event_set(myevent_s *ev, int fd, int events, call_back fun, void *arg)
{
    ev->fd = fd;
    ev->events = events;
    ev->fun = fun;
    ev->arg = arg;
}

void event_add(int epfd, myevent_s *ev)
{
    struct epoll_event tep;
    tep.data.ptr = ev;
    tep.events = ev->events;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ev->fd, &tep) == -1)
        printf("Error: epoll_ctl add fd %d, event is %d\n", ev->fd, ev->events);
    else {
        ev->status = 1;
        ev->last_active_time = time(NULL);
    }
}

void event_del(int epfd, myevent_s *ev)
{
    ev->status = 0;
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, NULL) == -1)
        printf("Error: epoll_ctl del fd %d, event is %d\n", ev->fd, ev->events);
}

void close_cfd(int cfd, myevent_s *ev)
{
    char str[BUFSIZ];
    event_del(g_efd, ev);
    sprintf(str, "the client fd: %d is closed\n", ev->fd);
    write(STDOUT_FILENO, str, strlen(str));
    return;
}

// build connection with new client
void cb_accept(int lfd, void *arg)
{
    printf("cb_accept\n");
    fflush(stdout);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int cfd = accept(lfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (cfd == -1)
        sys_error("accept error");
    int i = 0;
    for (; i < MAX_EVENTS && g_events[i].status != 0; ++i);
    if (i == MAX_EVENTS) {
        printf("the client num is max\n");
        return;
    }
    struct myevent_s *ev = &g_events[i];
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    
    event_set(ev, cfd, EPOLLIN | EPOLLET, login_menu, ev);
    event_add(g_efd, ev);
    printf("the new client ip is %s, the client port is %d\n", 
           inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ev->buf, sizeof ev->buf),
           ntohs(client_addr.sin_port));
    write(cfd, login_hint, sizeof(login_hint));
}

void cb_write(int cfd, void *arg)
{
    myevent_s *ev = (myevent_s*)arg;
    if (ev->len <= 0) {
        logout(cfd, ev);
        close_cfd(cfd, ev);
        return;
    }
    for(int i = r[0]; i != 1; i = r[i]) {
        if (online_fd[i] == cfd) continue;
        write(online_fd[i], ev->buf, ev->len);
    }
    if (ev->log_step == 3) write(cfd, "\n>>>", 4);
    event_del(g_efd, ev);
    event_set(ev, cfd, EPOLLIN | EPOLLET, cb_read, ev);
    event_add(g_efd, ev);
}

void cb_read(int cfd, void *arg) 
{
    char str[BUFSIZ - 259], str2[1024];
    myevent_s *ev = (myevent_s*)arg;
    int ret = read(cfd, str, sizeof(str));
    if (ret <= 0) {
        logout(cfd, ev);
        close_cfd(cfd, ev);
        return ;
    }
    str[ret] = '\0';
    sprintf(str2, "from client fd: %d receive data is :", cfd);
    if (ret > 0) write(STDOUT_FILENO, str2, strlen(str2));
    write(STDOUT_FILENO, str, ret);
    sprintf(ev->buf, "(%s):%s\n", ev->um.usr_name ,str);
    ev->len = strlen(ev->buf);

    event_del(g_efd, ev);
    event_set(ev, cfd, EPOLLOUT, cb_write, ev);
    event_add(g_efd, ev);
}

int main(int argc, char *argv[])
{
    printf("main\n");
    list_init();
    load_usermsg();
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) 
        sys_error("socket error!");

    int flag = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) 
        sys_error("setsocket error!");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(lfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        sys_error("bind error!");

    if (listen(lfd, 128) == -1)
        sys_error("listen error!");

    g_efd = epoll_create(MAX_EVENTS + 1);
    if (g_efd == -1)
        sys_error("epoll_create error!");

    for (int i = 0; i < MAX_EVENTS; ++i) 
        g_events[i].status = 0;

    myevent_s *lev = &g_events[MAX_EVENTS];
    event_set(lev, lfd, EPOLLIN | EPOLLET, cb_accept, lev);
    event_add(g_efd, lev);
    struct epoll_event eps[MAX_EVENTS + 1];
    int check_active = 0;
    char t[] = "-------------------server start-------------------\n";
    write(STDOUT_FILENO, t, sizeof(t));

    while (1)
    {
        long long now_time = time(NULL);
        for (int i = 0; i < 100; ++i, ++check_active)
        {
            if (check_active == MAX_EVENTS)
                check_active = 0;
            if (g_events[check_active].status != 1)
                continue;
            if (now_time - g_events[check_active].last_active_time >= 60) {
                char *buf = g_events[check_active].buf;
                sprintf(buf,"-----------------太长时间未操作,已与服务器断开连接--------------------\n");
                write(g_events[check_active].fd, buf, strlen(buf));
                if (g_events[check_active].log_step == 3)
                    logout(g_events[check_active].fd, &g_events[check_active]);
                close_cfd(g_events[check_active].fd, &g_events[check_active]);
            }
        }
        int num = epoll_wait(g_efd, eps, MAX_EVENTS + 1, 500);

        for (int i = 0; i < num; i++)
        {
            myevent_s *ev = (myevent_s*)eps[i].data.ptr;
            ev->fun(ev->fd, ev->arg);
        }
    }
    close(lfd);
    return 0;
}