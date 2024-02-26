/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void doit(int fd)
{
  int is_static;            // 정적 파일 또는 동적 파일 Check 변수
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char fileName[MAXLINE], cgiArgs[MAXLINE];
  rio_t rio;

  // Request Header의 정보를 읽고 buf에 저장한다.
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  // case1. GET 요청이 아닐시
  if(strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio);

  // 요청들어온 uri를 파싱한다.
  is_static = parse_uri(uri, fileName, cgiArgs);
  if(stat(fileName, &sbuf) < 0)
  {
    clienterror(fd, fileName, "404", "Not Found", "Tiny couldn't read the file");
    return;
  }

  // 정적 파일일시
  if(is_static)
  {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, fileName, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, fileName, sbuf.st_size);
  }
  else
  {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, fileName, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, fileName, cgiArgs);
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // 빈 줄이 나올 때까지 buf에 요청 헤더 정보를 읽는다.
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *fileName, char *cgiargs)
{
  char *ptr;

  // 정적 파일일 때, (CGI 파일이 없을때)
  if(!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");
    strcpy(fileName, ".");
    strcat(fileName, uri);
    if(uri[strlen(uri) - 1] == '/')
      strcat(fileName, "home.html");
    return 1;
  }
  // 동적 파일일 때,
  else
  {
    ptr = index(uri, '?');
    if(ptr)
    {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");

    strcpy(fileName, ".");
    strcat(fileName, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, fileType[MAXLINE], buf[MAXBUF];

  get_filetype(filename, fileType);                           // 응답 헤더 정보 클라이언트에게 반환
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-Length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-Type: %s\r\n", buf, fileType);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);


  srcfd = Open(filename, O_RDONLY, 0);                        // 응답 결과물 클라이언트에게 반환
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptyList[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0)                             // CGI 프로세스 생성
  {
    setenv("QUERY_STRING", cgiargs, 1);       // 환경 변수 설정
    Dup2(fd, STDOUT_FILENO);                  // 클라이언트에게 결과를 전달해주기위해 클라이언트 서버에 표준 출력 설정
    Execve(filename, emptyList, environ);
  }
  Wait(NULL);                                 // 좀비프로세스 생성을 막기 위해 자식 프로세스 종료될 때까지 대기
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  //response 쓰기
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);        // 버전, 에러번호, 상태메시지
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));                          // body 입력
}

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}