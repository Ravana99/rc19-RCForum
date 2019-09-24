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
    char buffer[128];
    
    if (gethostname(buffer, 128) == -1)
        fprintf(stderr, "error: %s\n", strerror(errno));
    else 
        printf("host name: %s\n", buffer);
    
    exit(0);
}