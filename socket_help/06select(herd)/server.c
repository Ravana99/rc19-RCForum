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

#define max(A, B) ((A) >= (B) ? (A) : (B))

void exit1(char* s) {
    fprintf(stderr, "%s: %s\n", s, strerror(errno));
    exit(1);
}

int main() {

    int fd, newfd, errcode;
    int maxfd, counter;
    int afd = 0;
    fd_set rfds;
    enum {idle, busy} state;
    struct sockaddr_in addr;
    struct addrinfo hints, *res;
    socklen_t addrlen;
    ssize_t n, nw;
    char *ptr, buffer[128];
    struct sigaction act;
    
    memset(buffer, 0, sizeof buffer);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &act, NULL) == -1) exit(1);

    if ((errcode = getaddrinfo(NULL, "58001", &hints, &res)) != 0) exit(1);
    
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) exit1("socket");
/**/if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit1("setsockopt");
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) exit1("bind");
    if (listen(fd, 5) == -1) exit1("listen");

    state = idle;
    while(1) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        maxfd = fd;
        if (state == busy) {
            FD_SET(afd, &rfds);
            maxfd = max(maxfd, afd);
        }
        if ((counter = select(maxfd + 1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL)) <= 0) exit(1);

        if (FD_ISSET(fd, &rfds)) {
            addrlen = sizeof(addr);
            if ((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit1("accept");
            switch (state) {
                case idle:
                    afd = newfd;
                    state = busy;
                    break;
                case busy:
                    write(newfd, "busy\n", 5);
                    close(newfd);
                    break;
            }
        }

        if (FD_ISSET(afd, &rfds)) {
            while ((n = read(newfd, buffer, 128)) != 0) {
                if (n == -1) exit1("read");
                ptr = &buffer[0];
                while (n > 0) {
                    if ((nw = write(newfd, ptr, n)) <= 0) exit1("write");
                    n -= nw;
                    ptr += nw;
                }
            }
            if (n == 0) {
                close(afd);
                state = idle;
            }

        write(1, "received: ", 10);
        write(1, buffer, strlen(buffer));

        }
    }   

    freeaddrinfo(res);
    close(fd);

    exit(0);
}

