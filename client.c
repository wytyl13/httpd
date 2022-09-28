/**********************************************************************
 * Copyright (C) 2022. IEucd Inc. All rights reserved.
 * @Author: weiyutao
 * @Date: 2022-09-28 17:13:04
 * @Last Modified by: weiyutao
 * @Last Modified time: 2022-09-28 17:13:04
 * @Description: the client model to test the server
***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("./client port\n");
        exit(1);
    }
    //create the strem socket fd_c what is used to connect to server
    int fd_c = socket(AF_INET, SOCK_STREAM, 0);
    char buf[BUFSIZ];

    //here we should bind the server address struct
    struct sockaddr_in addr_s;
    memset(&addr_s, 0, sizeof(addr_s));
    addr_s.sin_family = AF_INET;
    addr_s.sin_port = htons(8000);
    inet_pton(AF_INET, "192.168.5.7", &addr_s.sin_addr.s_addr);
    //notice:here we do not need to bind the client address struct,
    //it is because system help to implicit binding.

    if (connect(fd_c, (struct sockaddr *)&addr_s, sizeof(addr_s)))
        perror("connect error\n");
        exit(1);

    while (1)
    {
        //business process
    }
    return 0;
}
