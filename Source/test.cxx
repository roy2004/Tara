#include <string.h> // bzero, strlen
#include <netinet/in.h> // struct sockaddr_in, htonl, htons
#include <sys/socket.h> // socket, blind
#include <unistd.h> // write, close

#include "Runtime.hxx"

int Main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    struct sockaddr_in servaddr;
    int listenfd;
    int flag;

    bzero(&servaddr, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 服务器IP地址
    servaddr.sin_port = htons(4399); // 服务器TCP端口号

    listenfd = Tara::Socket(servaddr.sin_family, SOCK_STREAM, 0);

    flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    bind(listenfd, (struct sockaddr *)&servaddr, sizeof servaddr);
    listen(listenfd, 1); // {*:4399, }

    unsigned long i = 0;
    for (; ; ) {
        int connfd;
        connfd = Tara::Accept4(listenfd, NULL, NULL, 0, -1); // {, *:*}
        ++i;
        printf("[%lu] begin\n", i);

        Tara::Call([connfd, i] {
          const char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\n<h1>Hello</h1>\r\n";
          Tara::Write(connfd, msg, strlen(msg) + 1, -1);
          Tara::Close(connfd);
          printf("[%lu] end\n", i);
        });
    }
    Tara::Close(listenfd);

    return 0;
}
