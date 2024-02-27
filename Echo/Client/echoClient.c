#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = open_clientfd(host, port);                       // Getaddrinfo, socket, connect를 시켜준다. 호스트와 포트값을 설정한다.
    Rio_readinitb(&rio, clientfd);

    // 입력이 없을 때까지 Rio 패키지를 통해 입력된 문자열을 버퍼에 담아 요청한다.
    while(Fgets(buf, MAXLINE, stdin) != NULL)
    {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }

    Close(clientfd);
    exit(0);
}