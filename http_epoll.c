/**********************************************************************
 * Copyright (C) 2022. IEucd Inc. All rights reserved.
 * @Author: weiyutao
 * @Date: 2022-09-29 16:33:38
 * @Last Modified by: weiyutao
 * @Last Modified time: 2022-09-29 16:33:38
 * @Description: 
 * @Description: the http server using epoll to manager the pthread what here means the 
 * socket stream(tcp).not post method. epoll,used red-black trees to manage sockets that a 
 * file description bind the server or client address struct.why epoll?first it is the enhanced
 * version of poll and select.it can improve cpu usage in the term of concurrent connections.
 * second,because of the using of red-black data structure, so only consider the file descriptors
 * that are woken up.of course,one fd is a connection.Although epollmanages fd directly,but the fd 
 * here bind the socket what a connected thread,we can think of it as a thread.the thread design cater 
 * for cpu switching context,make it possible that a server can handle high concurrency.the thread in 
 * linux is the evolution of process.the linux opend some interfaces for thread manipulation.so we can 
 * conclude that the epoll,it is essentially a manager what use the red-black tree and the thread-dependent 
 * system call functions to efficent management the fd that server connected with every client.
***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>

#define MAXSIZE 2048
#define SERVER_STRING "Server: http server/0.1.0\r\n"
//teo macro sizes.first is standard bufsize,second is standard ip size.
char buf[BUFSIZ];
char client_ip[INET_ADDRSTRLEN];
//fault tolerance function
void error_die(const char *str) 
{
    perror(str);
    exit(1);
}

int init_listen_fd(int port, int fd_epoll)
{
    //sock_stream : tcp
    //sock_DGRAM : udp
    //AF_INET : net socket
    //AF_UNIX : host socket
    int ret, fd_s = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_s == -1)
        error_die("socket error\n");
    struct sockaddr_in addr_s, addr_c;
    //you should clear the address struct first
    memset(&addr_s, 0, sizeof(addr_s));
    // bzero(&addr_s, sizeof(addr_s));
    addr_s.sin_family = AF_INET;
    //s : port
    //l : addr
    //h : host
    //n : net
    addr_s.sin_port = htons(port);
    //INADDR_ANY : get the binary server address structure
    //htonl : transform the host binary structure to net
    addr_s.sin_addr.s_addr = htonl(INADDR_ANY);

    //port multiplexing
    int opt = 1;
    setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //bind the server address structure with fd_s
    //the fd_s dedicated to listening for client connections.
    if (bind(fd_s, (struct sockaddr *)&addr_s, sizeof(addr_s)) == -1)
        error_die("bind error\n");
    
    //set the max listening number
    if (listen(fd_s, 120) == -1)
        error_die("listen error\n");

    return fd_s;
}

//add the listen file descriptor fd_s or connection
//file descriptor fd_c.
int add_root_fd(int root, int fd, uint32_t event) 
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    return epoll_ctl(root, EPOLL_CTL_ADD, fd, &ev);
}

//set ET and non blocking mode using bitmap to 
//change the fd state in the linux kernel
//we should get the flag what is the default in kernel.
//then change the flag to non blocking mode.
//last,for bitting operation between flag and bitmap using fcntl.
//only in this way can we change the kernel default blocking mode. 
void et_non_blocking(int fd_c) 
{
    int flag = fcntl(fd_c, F_GETFL);
    flag != O_NONBLOCK;
    fcntl(fd_c, F_SETFL, flag);
}

/**
 * @Author: weiyutao
 * @Date: 2022-09-29 22:00:27
 * @Parameters: listening fd, root
 * @Return: NULL
 * @Description: the job of fd_s,conenct the server and client by fd. 
 * we should use system call function accept to establish the connection.
 * then give this fd to epoll and add corresponding events.
 */
void do_accept(int fd_s, int fd_epoll) 
{
    int fd_c;
    //define the address struct type object of client.
    //as the outgoing parameter to get the socket of client.
    //the addr_c_len is the capacity of addr_c,as the incoming
    //and outgoing parameters in accept function. 
    //the return fd_c is successful connected fd.
    struct sockaddr_in addr_c;
    socklen_t addr_c_len = sizeof(addr_c);
    fd_c = accept(fd_s, (struct sockaddr *)&addr_c, &addr_c_len);

    // we can get the client information from addr_c and echo it.
    printf("new client IP is %s, port is %d, fd_c is %d\n",
           inet_ntop(AF_INET, &addr_c.sin_addr.s_addr, client_ip, sizeof(client_ip)),
               ntohs(addr_c.sin_port),
           fd_c);
    
    //set ET and non blocking mode.
    et_non_blocking(fd_c);
    //the fd_c what to put on the tree and set the non blocking mode.
    if (add_root_fd(fd_epoll, fd_c, EPOLLIN | EPOLLET) == -1)
        error_die("epoll_ctl fd_c error\n");
}

const char *get_file_type(const char *name)
{
    char *dot;
    dot = strrchr(name, '.'); //从右开始扫描字符串name中的点,返回从右开始到第一个点的字符串
    if (dot == NULL)
    {
        return "text/plain; charset=utf-8";
    }
    if (strcmp(dot, ".html") == 0)
    {
        return "text/html; charset=utf-8";
    }
    if (strcmp(dot, ".jpg") == 0)
    {
        return "image/jpeg";
    }
    if (strcmp(dot, ".png") == 0)
    {
        return "image/png";
    }
    if (strcmp(dot, ".avi") == 0)
    {
        return "video/x-msvideo";
    }
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    {
        return "video/quicktime";
    }
    if (strcmp(dot, "mp3") == 0)
    {
        return "audio/mpeg";
    }
    return "text/plain; charset=utf-8";
}

void disconnect(int fd_c, int fd_epoll)
{
    int ret = epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_c, NULL);
    if(ret != 0)
        error_die("epoll_ctl error\n");
    close(fd_c);
}

//read file and send,dedicated by function send_file
void cat_close(int fd_c, int fd) 
{
    int n = 0;
    char buf[4096];
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        //equal to write if the last param is zero.
        if (send(fd_c, buf, n, 0) == -1)
        {
            printf("send error, errno =  %d\n", errno);
            if (errno == EAGAIN)
                continue;
            else if (errno == EINTR)
                continue;
            else
            {
                error_die("send error\n");
            }
        }
    }
    close(fd);
}

//404 not found
void not_found(int client)
{
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    sprintf(buf + strlen(buf), SERVER_STRING);
    sprintf(buf + strlen(buf), "Content-Type: text/html\r\n");
    sprintf(buf + strlen(buf), "\r\n");
    sprintf(buf + strlen(buf), "<HTML><TITLE>Not Found</TITLE>\r\n");
    sprintf(buf + strlen(buf), "<p style=\"text-align:center;\"><img src=\"not_found_404.png\" width=\"400\" height=\"300\" margin:auto /></p>\r\n");
    sprintf(buf + strlen(buf), "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

//send the respond header
void send_respond(int fd_c, int no, char *descripe, const char *type, int len)
{
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, descripe);
    sprintf(buf + strlen(buf), "%s\r\n", type);
    sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
    send(fd_c, buf, strlen(buf), 0);
    send(fd_c, "\r\n", 2, 0);
}

//read file and write buf accross the fd_c
void send_file(int fd_c, const char *file){
    int ret, n;
    int fd = open(file, O_RDONLY);
    if(fd == -1)
        not_found(fd_c);
    //cat and stop listening from epoll
    cat_close(fd_c, fd);
}

//read dir and send across fd_c in terms of html
void send_dir(int fd_c, const char *dirname)
{
    int i, ret;
    char buf[4094] = {0};
    sprintf(buf, "<html><head><title>dir_name : %s</title></head>", dirname);
    sprintf(buf + strlen(buf), "<body><h1>current_dir : %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};

    struct dirent **ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);

    for (i = 0; i < num; i++)
    {
        char *name = ptr[i]->d_name;
        struct stat st;
        stat(path, &st);
        if (strcmp(dirname, "./") != 0)
        {
            sprintf(path, "%s/%s", dirname, name);
        }
        else
        {
            sprintf(path, "%s%s", dirname, name);
        }
        printf("%s %s %s\n", dirname, name, path);
        if (S_ISREG(st.st_mode))
        {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"/%s\">%s</a></td><td>%ld</td></tr>", 
            path, name, (long)st.st_size);
        }
        else if (S_ISDIR(st.st_mode))
        {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s/</a></td><td>%ld</td></tr>", 
            path, name, (long)st.st_size);
        }
        ret = send(fd_c, buf, strlen(buf), 0);
        if (ret == -1)
        {
            if (errno == EAGAIN)
            {
                perror("send error\n");
                continue;
            }
            else if (errno == EINTR)
            {
                perror("send error\n");
                continue;
            }
            else
            {
                perror("send error\n");
                exit(1);
            }
        }
        memset(buf, 0, sizeof(buf));
    }
    sprintf(buf + strlen(buf), "</table></body></html>");
    send(fd_c, buf, strlen(buf), 0);

    printf("dir message send OK!\n");
}

//read one line in a file that end with the \r\n
int get_line(int fd_c, char *buf, int size){
    int i = 0;
    char c = '\0';
    int n;
    while((i < size -1) && (c != '\n')){
        n = recv(fd_c, &c, 1, 0);
        if(n > 0){
            if(c == '\r'){
                n = recv(fd_c, &c, 1, MSG_PEEK);
                if((n > 0) && (c == '\n')){
                    recv(fd_c, &c, 1, 0);
                }else{
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }else{
            c = '\n';
        }
    }
    buf[i] = '\0';

    if(-1 == n){
        i = n;
    }
    return i;
}

void do_read(int fd_c, int fd_epoll){
    char line[1024];
    struct stat st;
    //if return zero,mean client close.
    int ret = get_line(fd_c, line, sizeof(line));
    if (ret == 0)
    {
        printf("server test the client closed...\n");
        disconnect(fd_c, fd_epoll);
    }
    else
    {
        char method[16], path[256], protocal[16], buf[1024] = {0};
        sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocal);
        printf("method=%s, path=%s, protocal=%s",method, path, protocal);
        while (1)
        {
            char buf[1024] = {0};
            int len = get_line(fd_c, buf, sizeof(buf));
            if (buf[0] == '\n')
            {
                break;
            }
            else if (len == -1)
            {
                break;
            }
        }

        if (strncasecmp(method, "GET", 3) == 0)
        {
            char *file = path + 1;
            if(path[strlen(path) - 1] ==   '/'){
                file = "./";
            }
            if (stat(file, &st) == -1)
            {
                not_found(fd_c);
            }
            if (S_ISREG(st.st_mode))
            {
                // generel file
                send_respond(fd_c, 200, "OK", get_file_type(file), st.st_size);
                send_file(fd_c, file);
            }
            else if (S_ISDIR(st.st_mode))
            {
                // dir
                send_respond(fd_c, 200, "OK", get_file_type(file), -1);
                send_dir(fd_c, file);
            }
        }
        disconnect(fd_c, fd_epoll);
    }
}

/**
 * @Author: weiyutao
 * @Date: 2022-09-29 16:49:21
 * @Parameters: the PORT received from terminal
 * @Return: NULL
 * @Description : the second main function,involved all process run.
 */
void start_run(int port) 
{
    struct epoll_event all_events[MAXSIZE];

    //create the root,you should use the maxsize
    //that mean the max listening number
    int i, ret, fd_epoll = epoll_create(MAXSIZE);
    if (fd_epoll == -1)
        error_die("epoll_create error\n");

    //create the listening fd fd_s,you should use the
    //port and root
    int fd_s = init_listen_fd(port, fd_epoll);
    //make fd_s put on the tree
    if (add_root_fd(fd_epoll, fd_s, EPOLLIN) == -1)
        error_die("epoll_ctl fd_s error\n");
    while (1)
    {
        //the fourth param -1 means blocking and listening
        //all_events is the outgoing param
        //fd_epoll is the root
        //MAXSIZE is the initialize capacity of all_events
        //the return value is the number of wake events.involves
        //connect or data transform.block the listener first.if no
        //wake events.process will stay here for ever.
        if ((ret = epoll_wait(fd_epoll, all_events, MAXSIZE, -1)) == -1)
            error_die("epoll_wait error\n");
        //the program can run here,ret must greater than zero.
        for (i = 0; i < ret; ++i)
        {
            //go through each event and handle it
            //when it come events,it could be a connection or a business
            //we should judge it.but it could not the write event.
            //because the server process should receive the read event first.
            struct epoll_event *epv = &all_events[i];
            //bit operations are used here to judge and ignore not read event.
            if (!(epv->events & EPOLLIN))
                continue;
            //if the fd of event is fd_s,mean we should create connection.
            if (epv->data.fd == fd_s)
            {
                do_accept(fd_s, fd_epoll);
            }
            else
            {
                //else we should read the fd_c,means to handle business process.
                do_read(epv->data.fd, fd_epoll);
            }
        }
    }
}   

/**
 * @Author: weiyutao
 * @Date: 2022-09-29 16:35:28
 * @Parameters: 
 * @Return: 
 * @Description: the entrance
 */
int main(int argc, char const *argv[])
{
    struct sockaddr_in addr_s;
    if (argc < 3)
        error_die("./server port path\n");
    int port = atoi(argv[1]);
    int ret = chdir(argv[2]);
    if (ret != 0)
        error_die("chdir error\n");
    //the second main function
    start_run(port);        
    return 0;
}
