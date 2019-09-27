#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define PORT "58001"
#define MAXBUFFER 512
#define max(A, B) ((A) >= (B) ? (A) : (B))

void exit1(char* s) {
    fprintf(stderr, "%s: %s\n", s, strerror(errno));
    exit(1);
}

int main() {

    int listenfd, connfd, udpfd, nready, maxfdp1, errcode, n;
    char buffer[MAXBUFFER];
    fd_set rset;
    struct sockaddr_in cliaddr;
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    socklen_t len;
    ssize_t nw;
    char *ptr;
    struct sigaction act, act2;
    pid_t childpid;
    
    memset(buffer, 0, sizeof buffer);

    memset(&hintstcp, 0, sizeof hintstcp);
    hintstcp.ai_family = AF_INET;
    hintstcp.ai_socktype = SOCK_STREAM;
    hintstcp.ai_flags = AI_PASSIVE|AI_NUMERICSERV;
    memset(&hintsudp, 0, sizeof hintsudp);
    hintsudp.ai_family = AF_INET;
    hintsudp.ai_socktype = SOCK_DGRAM;
    hintsudp.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) exit1("sigaction");
    memset(&act2, 0, sizeof act);
    act2.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act2, NULL) == -1) exit1("sigaction");

    if ((errcode = getaddrinfo(NULL, PORT, &hintstcp, &restcp)) != 0) exit1("getaddrinfo");
    if ((errcode = getaddrinfo(NULL, PORT, &hintsudp, &resudp)) != 0) exit1("getaddrinfo");
    

    /* tcp socket */
    if ((listenfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1) exit1("socket");
  //if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit1("setsockopt");
    if (bind(listenfd, restcp->ai_addr, restcp->ai_addrlen) == -1) exit1("bind");
    if (listen(listenfd, 5) == -1) exit1("listen");

    /* udp socket */
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1) exit1("socket");
    if ((errcode = bind(udpfd, resudp->ai_addr, resudp->ai_addrlen)) == -1) exit1("bind");
  //if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit1("setsockopt");

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;

    while(1) {

        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        if ((nready = select(maxfdp1 + 1, &rset, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL)) <= 0) exit1("select");

        if (FD_ISSET(listenfd, &rset)) {

            len = sizeof(cliaddr);
            do connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len); while (connfd == -1 && errno == EINTR);
            if (connfd == -1) exit1("accept");
            
            if ((childpid = fork()) == 0) {
                close(listenfd);
                memset(buffer, 0, sizeof buffer);
                while ((n = read(connfd, buffer, 128)) != 0) {
                    if (n == -1) exit1("read");
                    ptr = &buffer[0];
                    while (n > 0) {
                        if ((nw = write(connfd, ptr, n)) <= 0) exit1("write");
                        n -= nw;
                        ptr += nw;
                    }
                }
                write(1, "received from tcp: ", 19);
                write(1, buffer, strlen(buffer));
                close(connfd);
                exit(0);
            }
            else if (childpid == -1) exit1("fork");
            do errcode = close(connfd); while (errcode == -1 && errno == EINTR);
            if (errcode == -1) exit1("close");

        }

        if (FD_ISSET(udpfd, &rset)) {
            len = sizeof(cliaddr);
            memset(buffer, 0, sizeof buffer);
            if ((n = recvfrom(udpfd, buffer, sizeof buffer, 0, (struct sockaddr*)&cliaddr, &len)) == -1) exit1("recvfrom");
            write(1, "received from udp: ", 19);
            write(1, buffer, strlen(buffer));
            if ((n = sendto(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, len)) == -1) exit1("sendto");
        }
    }   

    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(listenfd);
    close(udpfd);

    exit(0);
}
