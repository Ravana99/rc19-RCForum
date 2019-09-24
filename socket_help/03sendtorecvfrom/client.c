#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    struct addrinfo hints, *res;
    int fd, errcode;
    ssize_t n;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char hostname[128];
    char buffer[128];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;
   
    
    if (gethostname(hostname, 128) == -1) exit(1);

    if ((errcode = getaddrinfo(hostname, "58001", &hints, &res)) != 0) exit(1);

    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) exit(1);

    if ((n = sendto(fd, "Hello!\n", 7, 0, res->ai_addr, res->ai_addrlen)) == -1) exit(1);

    addrlen = sizeof(addr);
    if ((n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);

    write(1, "echo: ", 6);
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);



    exit(0);
}