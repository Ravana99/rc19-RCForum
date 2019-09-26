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
#include <unistd.h>

#define PORT "58001"

int main() {
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    ssize_t n, nbytes, nleft, nwritten, nread;
    int tcpfd, udpfd, errcode, userid = -1, topicnum = -1;
    struct sigaction act;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char hostname[128], buffer[128], received[128], command[128], argbuffer[128], input[1024];
    char *inputptr, *ptr;


    memset(buffer, 0, sizeof buffer);
    memset(received, 0, sizeof received);

    memset(&hintstcp, 0, sizeof hintstcp);
    hintstcp.ai_family = AF_INET;
    hintstcp.ai_socktype = SOCK_STREAM;
    hintstcp.ai_flags = AI_NUMERICSERV;
    memset(&hintsudp, 0, sizeof hintsudp);
    hintsudp.ai_family = AF_INET;
    hintsudp.ai_socktype = SOCK_DGRAM;
    hintsudp.ai_flags = AI_NUMERICSERV;

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
    
    if (gethostname(hostname, 128) == -1) exit(1);

    if ((errcode = getaddrinfo(hostname, PORT, &hintstcp, &restcp)) != 0) exit(1);
    if ((errcode = getaddrinfo(hostname, PORT, &hintsudp, &resudp)) != 0) exit(1);

    while(1) {
        fgets(input, 1024, stdin);
        if (!strcmp(input, "\n")) continue;
        sscanf(input, "%s", command);
        inputptr = input;
        inputptr += strlen(command) + 1;

        if (!strcmp(command, "register") || !strcmp(command, "reg")) {
            sscanf(inputptr, "%s", argbuffer);
            if ((userid = atoi(argbuffer)) == 0) {
                printf("Invalid arguments\n");
                continue;
            }

            //TODO: NOT YET IMPLEMENTED
            printf("User \"%d\" registered\n", userid);

        } else if (userid == -1) {
            printf("User not registered\n");
            continue;

        } else if (!strcmp(command, "topic_list") || !strcmp(command, "tl")) {
            //TODO: NOT YET IMPLEMENTED
            printf("[TOPICS]\n");

        } else if (!strcmp(command, "topic_select") || !strcmp(command, "ts")) {
            sscanf(inputptr, "%s", argbuffer);
            if ((topicnum = atoi(argbuffer)) == 0) {
                printf("Invalid arguments\n");
                continue;
            }

            //TODO: NOT YET IMPLEMENTED
            printf("TOPIC SELECTED: %d\n", topicnum);

        } else if (!strcmp(command, "topic_propose") || !strcmp(command, "tp")) {
            sscanf(inputptr, "%s", argbuffer);

            //TODO: NOT YET IMPLEMENTED
            printf("TOPIC PROPOSED: %s\n", argbuffer);

        } else if (!strcmp(command, "question_list") || !strcmp(command, "ql")) {
            if (topicnum == -1) {
                printf("No topic selected\n");
                continue;
            }

            //TODO: NOT YET IMPLEMENTED
            printf("[LIST OF QUESTIONS OF TOPIC %d]\n", topicnum);

        } else if (!strcmp(command, "question_get") || !strcmp(command, "qg")) {
            //TODO: NOT YET IMPLEMENTED

        } else if (!strcmp(command, "question_submit") || !strcmp(command, "qs")) {
            //TODO: NOT YET IMPLEMENTED

        } else if (!strcmp(command, "answer_submit") || !strcmp(command, "as")) {
            //TODO: NOT YET IMPLEMENTED

        } else if (!strcmp(command, "exit")) break;

        else printf("Invalid command\n");

        inputptr = input;

        memset(argbuffer, 0, sizeof buffer);
        memset(input, 0, sizeof buffer);
        memset(command, 0, sizeof buffer);
    }

    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    //close(udpfd);

    exit(0);
}

/*

    if ((tcpfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1) exit(1);

    if ((errcode = connect(tcpfd, restcp->ai_addr, restcp->ai_addrlen)) == -1) exit(1);

    ptr = strcpy(buffer, "Hello!\n");
    nbytes = 7;

    nleft = nbytes;
    while (nleft > 0) {
        if ((nwritten = write(tcpfd, ptr, nleft)) <= 0) exit(1);
        nleft -= nwritten;
        ptr += nwritten;
    }

    ptr = received;
    nleft = nbytes;
    while (nleft > 0) {
        if ((nread = read(tcpfd, ptr, nleft)) == -1) exit(1);
        else if (nread == 0) break;
        nleft -= nread;
        ptr += nread;
    }

    nread = nbytes-nleft;
    write(1, "echo from tcp: ", 15);
    write(1, received, nread);

    close(tcpfd);

    memset(buffer, 0, sizeof buffer);

    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1) exit(1);

    if ((n = sendto(udpfd, "Hi!\n", 4, 0, resudp->ai_addr, resudp->ai_addrlen)) == -1) exit(1);

    addrlen = sizeof(addr);
    if ((n = recvfrom(udpfd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);

    write(1, "echo from udp: ", 15);
    write(1, buffer, n);

*/