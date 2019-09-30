/* Group 39 - Joao Fonseca 89476, Tiago Pires 89544, Tomas Lopes 89552 */

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
#include <dirent.h>

#define max(A, B) ((A) >= (B) ? (A) : (B))

int receiveudp(int udpfd, char* buffer, struct sockaddr_in* cliaddr, socklen_t* len) {
    char request[128], response[1024];
    int id, n;

    if ((n = recvfrom(udpfd, buffer, 1024, 0, (struct sockaddr*)cliaddr, len)) == -1) return -1;
    buffer[n] = '\0'; // Appends a '\0' to the message so it can be used in strcmp
    sscanf(buffer, "%s", request);

    if (!strcmp(request, "REG")) {
        sscanf(buffer, "REG %d", &id);
        printf("User %d (IP %s) trying to register... ", id, inet_ntoa(cliaddr->sin_addr));
        if (id > 9999 && id < 100000) {
            strcpy(response, "RGR OK\n");
            printf("success\n");
        }
        else {
            strcpy(response, "RGR NOK\n");
            printf("denied\n");
        }

    } else if (!strcmp(request, "LTP")) {

        DIR *dir;
	    struct dirent *entry;
        int n = 0;
        char topic_amount[16], topic_buffer[128], topic_name[16], topic_id[16];
        char delim[2] = "_";

        printf("User is listing the available topics\n");
        strcpy(response, "LTR ");

	    if ((dir = opendir("server/topics")) == NULL) return -1;
		while ((entry = readdir(dir)) != NULL) n++;
        n -= 2; // Disregards directories . and ..
		closedir(dir);

        sprintf(topic_amount, "%d", n);
        strcat(response, topic_amount);

        if ((dir = opendir("server/topics")) == NULL) return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
		while ((entry = readdir(dir)) != NULL) {
            strtok(entry->d_name, delim);
            strcpy(topic_name, strtok(NULL, delim));
            strcpy(topic_id, strtok(NULL, delim));
            sprintf(topic_buffer, " %s:%s", topic_name, topic_id);
            strcat(response, topic_buffer);
        }
        strcat(response, "\n");

		closedir(dir);

                
    } else if (!strcmp(request, "PTP")) {

    } else if (!strcmp(request, "LQU")) {

    } else {
        strcpy(response, "ERR\n");
    }

    return sendto(udpfd, response, strlen(response) + 1, 0, (struct sockaddr*)cliaddr, *len);
}

int main(int argc, char** argv) {

    int listenfd, connfd, udpfd, nready, maxfdp1, errcode, n;
    char buffer[1024], port[16];
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
    if (sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
    memset(&act2, 0, sizeof act);
    act2.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act2, NULL) == -1) exit(1);

    strcpy(port, "58039");
    if (argc == 3) {
        if (!strcmp(argv[1], "-p"))
            strcpy(port, argv[2]);
        else {
            printf("Invalid command line arguments\n");
            exit(1);
        }
    } else if (argc != 1) {
        printf("Invalid command line arguments\n");
        exit(1);
    }

    if ((errcode = getaddrinfo(NULL, port, &hintstcp, &restcp)) != 0) exit(1);
    if ((errcode = getaddrinfo(NULL, port, &hintsudp, &resudp)) != 0) exit(1);
    

    /* tcp socket */
    if ((listenfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1) exit(1);
  //if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit(1);
    if (bind(listenfd, restcp->ai_addr, restcp->ai_addrlen) == -1) exit(1);
    if (listen(listenfd, 5) == -1) exit(1);

    /* udp socket */
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1) exit(1);
    if ((errcode = bind(udpfd, resudp->ai_addr, resudp->ai_addrlen)) == -1) exit(1);
  //if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit(1);

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;

    while(1) {

        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        if ((nready = select(maxfdp1 + 1, &rset, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL)) <= 0) exit(1);

        if (FD_ISSET(listenfd, &rset)) {
            /*
            len = sizeof(cliaddr);
            do connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len); while (connfd == -1 && errno == EINTR);
            if (connfd == -1) exit(1);
            
            if ((childpid = fork()) == 0) {
                close(listenfd);
                memset(buffer, 0, sizeof buffer);
                while ((n = read(connfd, buffer, 128)) != 0) {
                    if (n == -1) exit(1);
                    ptr = &buffer[0];
                    while (n > 0) {
                        if ((nw = write(connfd, ptr, n)) <= 0) exit(1);
                        n -= nw;
                        ptr += nw;
                    }
                }
                write(1, "received from tcp: ", 19);
                write(1, buffer, strlen(buffer));
                close(connfd);
                exit(0);
            }
            else if (childpid == -1) exit(1);
            do errcode = close(connfd); while (errcode == -1 && errno == EINTR);
            if (errcode == -1) exit(1);
            */
        }

        if (FD_ISSET(udpfd, &rset)) {
            len = sizeof(cliaddr);
            memset(buffer, 0, sizeof buffer);
            if (receiveudp(udpfd, buffer, &cliaddr, &len) == -1) exit(1);
        }
    }   

    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(listenfd);
    close(udpfd);

    exit(0);
}
