/*
 * echoserver.c - A simple connection-based echo server
 * usage: echoserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Runtime.hxx"

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(const char *msg) {
  perror(msg);
  exit(1);
}

int Main(int argc, char **argv) {
  int listenfd; /* listening socket */
  int portno; /* port to listen on */
  struct sockaddr_in serveraddr; /* server's addr */
  int optval; /* flag value for setsockopt */

  /* check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* socket: create a socket */
  listenfd = Tara::Socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT,
         (const void *)&optval , sizeof(int));

  /* build the server's internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET; /* we are using the Internet */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr.sin_port = htons((unsigned short)portno); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd, (struct sockaddr *) &serveraddr,
       sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(listenfd, 5) < 0) /* allow 5 requests to queue up */
    error("ERROR on listen");

  /* main loop: wait for a connection request, echo input line,
     then close connection. */
  while (1) {
    int connfd; /* connection socket */
    socklen_t clientlen; /* byte size of client's address */
    struct sockaddr_in clientaddr; /* client addr */
    char *hostaddrp; /* dotted decimal host addr string */

    /* accept: wait for a connection request */
    clientlen = sizeof(clientaddr);
    connfd = Tara::Accept4(listenfd, (struct sockaddr *) &clientaddr, &clientlen, 0, -1);
    if (connfd < 0)
      error("ERROR on accept");

    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    // printf("server established connection with %s\n", hostaddrp);

    Tara::Call([connfd] {
        char buf[BUFSIZE]; /* message buffer */
        int n; /* message byte size */

        /* read: read input string from the client */
        for (;;) {
          n = Tara::Read(connfd, buf, BUFSIZE, -1);
          if (n < 0)
            error("ERROR reading from socket");
          // printf("server received %d bytes: %s", n, buf);
          if (n == 0) {
            break;
          }

          /* write: echo the input string back to the client */
          n = Tara::Write(connfd, buf, strlen(buf), -1);
          if (n < 0)
            error("ERROR writing to socket");
        }

        Tara::Close(connfd);
    });
  }
}
