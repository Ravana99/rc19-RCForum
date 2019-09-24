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
    struct addrinfo hints, *res, *p;
    int errcode;
    char buffer[INET_ADDRSTRLEN];
    struct in_addr *addr;
    char hostname[128];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;  
    
    if (gethostname(hostname, 128) == -1)
        fprintf(stderr, "error: %s\n", strerror(errno));

    if ((errcode = getaddrinfo("RavanaPC", NULL, &hints, &res)) != 0)
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(errcode));
    else {
        printf("canonical hostname: %s\n", res->ai_canonname);
        for (p = res; p != NULL; p = p->ai_next) {
            addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
            printf("internet address: %s (%08lX)\n", inet_ntop(p->ai_family, addr, buffer, sizeof buffer), (long unsigned int)ntohl(addr->s_addr));
        }
        freeaddrinfo(res);
    }
    exit(0);
}