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

    int fd, errcode;
    struct sockaddr_in addr;
    struct addrinfo hints, *res;
    socklen_t addrlen;
    ssize_t n;
    char buffer[128];
    
    memset(buffer, 0, sizeof buffer);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((errcode = getaddrinfo(NULL, "58001", &hints, &res)) != 0) exit(1);

    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) exit(1);

    if ((n = bind(fd, res->ai_addr, res->ai_addrlen)) == -1) exit(1);

    addrlen = sizeof(addr);
    if ((n = recvfrom(fd, buffer, sizeof buffer, 0, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
        
    write(1, "received: ", 10);
    write(1, buffer, n);

    if ((n = sendto(fd, buffer, n, 0, (struct sockaddr*)&addr, addrlen)) == -1) exit(1);

    freeaddrinfo(res);
    close(fd);

    exit(0);
}