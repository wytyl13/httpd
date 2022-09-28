/**********************************************************************
 * Copyright (C) 2022. IEucd Inc. All rights reserved.
 * @Author: weiyutao
 * @Date: 2022-09-27 19:02:32
 * @Last Modified by: weiyutao
 * @Last Modified time: 2022-09-27 19:02:32
 * @Description: the simple http application not use the epoll what could handle multiple client 
 * connections,here we do not use the management epoll what is the thread or file description manager
 * //aim to learn the machine-processed of multiple thread.
***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <strings.h>
#include <dirent.h>

#define IsSpace(x) isspace((int)(x))
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
/**
 * @Author: weiyutao
 * @Date: 2022-09-27 18:50:06
 * @Parameters: a descripe string about the error message
 * @Return: NULL
 * @Description: Pront out an error message with perror and exit with 1.
 */
void error_die(const char *str) 
{
    perror(str);
    exit(1);
}

void send_respond(int fd_c, int no, char *descripe, const char *type, int len);
/**
 * @Author: weiyutao
 * @Date: 2022-09-27 19:11:53
 * @Parameters: the server port,need to define here
 * @Return: the socket, a listening file description
 * @Description: the function start the listening process for web connections on a specified
 * port.
 */
int startup(unsigned short *port) 
{
    //create the socket addr_s what is the server address
    //struct,then create the file description
    //used the socket function,the second param os stream sock what is the tcp.
    struct sockaddr_in addr_s;
    int fd_s = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if (fd_s == -1)
        error_die("socket");
    
    //clear
    memset(&addr_s, 0, sizeof(addr_s));
    addr_s.sin_family = AF_INET;
    addr_s.sin_port = htons(*port);
    addr_s.sin_addr.s_addr = htonl(INADDR_ANY);
    //port reuse
    if ((setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) < 0)
        error_die("setsocketopt error\n");
    
    //bind the fd_s to the addr_s and set the listen number
    if (bind(fd_s, (struct sockaddr *)&addr_s, sizeof(addr_s)) < 0)
        error_die("bind error\n");
    if (listen(fd_s, 120) < 0)
        error_die("listen error\n");

    return fd_s;
}

void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<p style=\"text-align:center;\"><img src=\"not_found_404.png\" width=\"400\" height=\"300\" margin:auto /></p>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
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

//close the fd_c
void disconnect(int fd_c) 
{
    close(fd_c);
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

//read file and send,dedicated by function send_file
void cat_close(int fd_c, int fd) 
{
    int n = 0;
    char buf[4096] = {0};
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

//read file and write buf accross the fd_c
void send_file(int fd_c, const char *file){
    int ret;
    int fd = open(file, O_RDONLY);
    if(fd == -1){
        error_die("open error\n");
    }
    cat_close(fd_c, fd);
}


//the next three function unused in this process
/* 
int hexit(char c){
    if(c >= '0' && c <= '9'){
        return c - '0';
    }
    if(c >= 'a' && c <= 'f'){
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F'){
        return c - 'A' + 10;
    }
    return 0;
} 
*/

/* 
void encode_str(char *to, int tosize, const char *from) 
{
    int tolen;
    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
    {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char *)0)
        {
            *to = *from;
            ++to;
            ++tolen;
        }
        else
        {
            sprintf(to, "%%%02x", (int)*from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
} 
*/

/* 
void decode_str(char *to, char *from) 
{
    for (; *from != '\0', ++to, ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            *to = hexit(from[1]) * 16 + hexit(from[2]);
            from += 2;
        }
        else
        {
            *to = *from;
        }
    }
    *to = '\0';
} 
*/

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

/**
 * @Author: weiyutao
 * @Date: 2022-09-27 21:51:23
 * @Parameters: the return value of accept function,a connected file descriotion betwwen server and client.
 * @Return: NULL
 * @Description: recall function,here we can get the http request content
 */
void accept_request(void *arg)
    {
        int fd_c = (int)arg;
        char buf[1024] = {0};
        char path[255];
        struct stat st;
        size_t numchars;
        size_t i, j;
        char method[255]; // get the GET or POST method
        char protocal[255];

        numchars = get_line(fd_c, buf, sizeof(buf));
        printf("%s\n", buf);
        // GET /picture3.jpg HTTP/1.1
        if (numchars == 0)
        //if the return value of read function is 0,it means the opposite
        //teminal closed.the read function is involved the get_line function
        //if it happend,server should disconnect the connection.
    {
        printf("server prevceived the client closed...");
        disconnect(fd_c);
        return;
    }
    i = 0;//the start index of method
    j = 0;
    while (!IsSpace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[i];
        i++;
    }
    j = i;
    method[i] = '\0';
    //jump the space,return the index j
    while (IsSpace(buf[j]) && (j < numchars))
        j++;
    //judge the space
    i = 0;//the start index of path
    while (!IsSpace(buf[j]) && (i < (sizeof(path) - 1)) && (j < (numchars - 1)))
    {
        path[i] = buf[j];
        i++; j++;
    }
    path[i] = '\0';

    i = 0;
    //jump the space
    while (IsSpace(buf[j]) && (j < numchars))
        j++;
    //judge the space
    while (!IsSpace(buf[j]) && (i < (sizeof(protocal) - 1)) && (j < (numchars - 1)))
    {
        protocal[i] = buf[j];
        i++; j++;
    }
    protocal[i] = '\0';
    printf("method = %s, path = %s, protocal = %s\n", method, path, protocal);
    if (strcasecmp(method, "GET") == 0)
    {
        //GET request:
        //notice:because the root directory has made in main function.so here
        //we need not to set it again.
        char *file = path + 1;
        if (path[strlen(path) - 1] ==   '/')
        {
            file = "./";
            // memset(path, 0, sizeof(path));
            // strcat(path, "index.html");
            // strcat(file, "index.html");
        }
        printf("%s\n", file);
        //notice:the next code is necessary. 
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(fd_c, buf, sizeof(buf));
        if (stat(file, &st) == -1)
        { // 404
            not_found(fd_c);
        }

        if (S_ISREG(st.st_mode))
        {
            //generel file
            send_respond(fd_c, 200, "OK", get_file_type(file), st.st_size);
            send_file(fd_c, file);
        }
        else if (S_ISDIR(st.st_mode))
        {
            //dir
            send_respond(fd_c, 200, "OK", get_file_type(file), -1);
            send_dir(fd_c, file);
        }
    }
    disconnect(fd_c);
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("./server port path\n");
        exit(1);
    }

    
    if (chdir(argv[2]) != 0)
    {
        error_die("chdir error\n");
    }
    unsigned short port = atoi(argv[1]);
    int fd_s = -1;
    int fd_c = -1;
    struct sockaddr_in addr_c;
    socklen_t addr_c_len = sizeof(addr_c);
    pthread_t newthread;
    
    fd_s = startup(&port);
    printf("httpd runing on port %d at dir %s\n", port, argv[2]);

    //come here we have created the server listen fd
    //then we should strat to listen,we will use while true
    //to circle listen the client connection.
    while (1)
    {
        //here fd_s what server listen fd will block the listenning
        //once received the client connect,the accept return the connected fd_c
        fd_c = accept(fd_s, (struct sockaddr *)&addr_c, &addr_c_len);
        if (fd_c == -1)
            error_die("accept error\n");
        
        //create a thread and define the recall function
        //set the param fd_c what is the connected fd to recall function
        //where can we get some request information about client,only own it 
        //can we get handle the business process.but the pthread_create function is base
        //on the fd_c what is the connected fd with client.only connected successful can we
        //use the pthread_create function to handle some business process.
        //so if the fd_c is not connected successful,the code will not run here.
        //if code run here,must has a connected successful client,so the list code
        //will create a pthread to handle request information about the connected client.
        if (pthread_create(&newthread, NULL, (void *)accept_request, (void *)fd_c) != 0)
        {
            error_die("pthread_create error\n");
        }
    }
    close(fd_s);
    return 0;
}