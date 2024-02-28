#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *new_version = "HTTP/1.0";

void do_it(int fd);
void do_request(int p_clientfd, char *method, char *uri_ptos, char *host);
void do_response(int p_connfd, int p_clientfd);
int parse_uri(char *uri, char *uri_ptos, char *host, char *port);
int parse_responsehdrs(rio_t *rp, int length);

int main(int argc, char **argv) { 
  int listenfd, p_connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if (argc != 2) {  
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  while(1) {
    clientlen = sizeof(clientaddr);

    p_connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    
    do_it(p_connfd); 
    Close(p_connfd);
  }
  return 0;
}

void do_it(int p_connfd){
  int p_clientfd;
  char buf[MAXLINE],  host[MAXLINE], port[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char uri_ptos[MAXLINE];
  rio_t rio;
  
  Rio_readinitb(&rio, p_connfd);
  Rio_readlineb(&rio, buf, MAXLINE); 
  printf("Request headers to proxy:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  parse_uri(uri, uri_ptos, host, port);

  p_clientfd = Open_clientfd(host, port);
  do_request(p_clientfd, method, uri_ptos, host);
  do_response(p_connfd, p_clientfd);        
  Close(p_clientfd); 
}

/* do_request: proxy => server */
void do_request(int p_clientfd, char *method, char *uri_ptos, char *host){
  char buf[MAXLINE];
  printf("Request headers to server: \n");     
  printf("%s %s %s\n", method, uri_ptos, new_version);

  /* Read request headers */        
  sprintf(buf, "GET %s %s\r\n", uri_ptos, new_version);
  sprintf(buf, "%sHost: %s\r\n", buf, host);   
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnections: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);

  Rio_writen(p_clientfd, buf, (size_t)strlen(buf));
}

/* do_response: server => proxy */
void do_response(int p_connfd, int p_clientfd){
  char buf[MAX_CACHE_SIZE];
  ssize_t n;
  rio_t rio;

  Rio_readinitb(&rio, p_clientfd);  //
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);   // robust ...~ MAXLINE까지 일단 다 읽음 
  Rio_writen(p_connfd, buf, n);
}

int parse_uri(char *uri, char *uri_ptos, char *host, char *port){ 
  char *ptr;

  if (!(ptr = strstr(uri, "://")))
    return -1;
  ptr += 3;                       
  strcpy(host, ptr);              // host = www.google.com:80/index.html

  if((ptr = strchr(host, ':'))){  // strchr(): 문자 하나만 찾는 함수 (''작은따옴표사용)
    *ptr = '\0';                  // host = www.google.com
    ptr += 1;
    strcpy(port, ptr);            // port = 80/index.html
  }
  else{
    if((ptr = strchr(host, '/'))){
      *ptr = '\0';
      ptr += 1;
    }
    strcpy(port, "80"); 
  }

  if ((ptr = strchr(port, '/'))){ // port = 80/index.html
    *ptr = '\0';                  // port = 80
    ptr += 1;     
    strcpy(uri_ptos, "/");        // uri_ptos = /
    strcat(uri_ptos, ptr);        // uri_ptos = /index.html
  }  
  else strcpy(uri_ptos, "/");

  return 0; // function int return => for valid check
}