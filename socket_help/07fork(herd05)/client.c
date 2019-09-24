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

int main() {
    struct addrinfo hints, *res;
    ssize_t nbytes, nleft, nwritten, nread;
    int fd, n;
    struct sigaction act;
    char hostname[128], *ptr, buffer[128], received[128];

    memset(buffer, 0, sizeof buffer);
    memset(received, 0, sizeof received);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
    
    if (gethostname(hostname, 128) == -1) exit(1);

    if ((n = getaddrinfo(hostname, "58001", &hints, &res)) != 0) exit(1);

    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) exit(1);

    if ((n = connect(fd, res->ai_addr, res->ai_addrlen)) == -1) exit(1);

    ptr = strcpy(buffer, "Hello!\n");
    nbytes = 7;

    nleft = nbytes;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) exit(1);
        nleft -= nwritten;
        ptr += nwritten;
    }

    ptr = received;
    nleft = nbytes;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) == -1) exit(1);
        else if (nread == 0) break;
        nleft -= nread;
        ptr += nread;
    }

    nread = nbytes-nleft;
    write(1, "echo: ", 6);
    write(1, received, nread);

    freeaddrinfo(res);
    close(fd);

    exit(0);
}