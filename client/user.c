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
#include <unistd.h>

struct topic
{
    char name[16];
    int id;
};

int sendudp(int udpfd, struct addrinfo **resudp, char *message, char *response)
{
    struct sockaddr_in addr;
    socklen_t addrlen;

    if (sendto(udpfd, message, strlen(message), 0, (*resudp)->ai_addr, (*resudp)->ai_addrlen) == -1)
        return -1;
    addrlen = sizeof(addr);
    return recvfrom(udpfd, response, 2048, 0, (struct sockaddr *)&addr, &addrlen);
}

int main(int argc, char **argv)
{
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    ssize_t n, nbytes, nleft, nwritten, nread;
    
    int tcpfd, udpfd, errcode, userid = -1, topic_number = 0,topic_amount = 0;
    struct sigaction act;

    char port[16], hostname[128], buffer[128], received[128], command[128], argbuffer[128], input[2048], message[2048], response[2048];
    char *inputptr, *ptr;

    struct topic topic_list[99 + 1]; // First entry is left empty to facilitate indexing
    memset(topic_list, 0, sizeof topic_list);

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

    // Ignoring the SIGPIPE signal
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
        exit(1);

    strcpy(hostname, "localhost");
    strcpy(port, "58039");
    if (argc == 3)
    {
        if (!strcmp(argv[1], "-n"))
            strcpy(hostname, argv[2]);
        else if (!strcmp(argv[1], "-p"))
            strcpy(port, argv[2]);
        else
        {
            printf("Invalid command line arguments\n");
            exit(1);
        }
    }
    else if (argc == 5)
    {
        if (!strcmp(argv[1], "-n") && !strcmp(argv[3], "-p"))
        {
            strcpy(hostname, argv[2]);
            strcpy(port, argv[4]);
        }
        else if (!strcmp(argv[1], "-p") && !strcmp(argv[3], "-n"))
        {
            strcpy(port, argv[2]);
            strcpy(hostname, argv[4]);
        }
        else
        {
            printf("Invalid command line arguments\n");
            exit(1);
        }
    }
    else if (argc != 1)
    {
        printf("Invalid command line arguments\n");
        exit(1);
    }

    if ((errcode = getaddrinfo(hostname, port, &hintstcp, &restcp)) != 0)
        exit(1);
    if ((errcode = getaddrinfo(hostname, port, &hintsudp, &resudp)) != 0)
        exit(1);

    if ((tcpfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
        exit(1);
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1)
        exit(1);

    while (1)
    {
        fgets(input, 2048, stdin);
        if (!strcmp(input, "\n"))
            continue;
        sscanf(input, "%s", command);
        inputptr = input;
        inputptr += strlen(command) + 1; // Points to arguments of the command

        if (!strcmp(command, "exit"))
            break;

        else if (!strcmp(command, "register") || !strcmp(command, "reg"))
        {
            sscanf(inputptr, "%s", argbuffer);
            userid = atoi(argbuffer);

            sprintf(message, "REG %s\n", argbuffer);
            if ((n = sendudp(udpfd, &resudp, message, response)) == -1)
                exit(1);
            response[n] = '\0'; // Appends a '\0' to the response so it can be used in strcmp

            if (!strcmp(response, "RGR OK\n"))
                printf("User \"%d\" registered\n", userid);
            else
            {
                printf("Registration failed\n");
                userid = -1;
            }
        }
        else if (!strcmp(command, "topic_list") || !strcmp(command, "tl"))
        {
            char old_topic[16];
            char delim[3] = ": ";
            int n;
            strcpy(old_topic, topic_list[topic_number].name);
            strcpy(message, "LTP\n");
            if ((n = sendudp(udpfd, &resudp, message, response)) == -1)
                exit(1);
            response[n] = '\0';
            printf("%s\n",strtok(response, delim); );
            strtok(response, delim);
            n = atoi(strtok(NULL, delim));
            topic_amount = n;
            for (int i = 1; i <= n; i++)
            {
                strcpy(topic_list[i].name, strtok(NULL, delim));
                topic_list[i].id = atoi(strtok(NULL, delim));
                if (!strcmp(old_topic, topic_list[i].name))
                    topic_number = i;
            }
            printf("Available topics:\n");
            for (int i = 1; i <= n; i++)
                printf("%d - %s (proposed by %d)\n", i, topic_list[i].name, topic_list[i].id);
        }
        else if (!strcmp(command, "topic_select"))
        {
            sscanf(inputptr, "%s", argbuffer);
            topic_number=0;
           	for (int i=1; i<=topic_amount;i++){
           		if (!strcmp(argbuffer, topic_list[i].name)){
           			printf("Selected topic: %s (proposed by %d)\n", topic_list[i].name, topic_list[i].id);
           			topic_number=i;
           			break;
           		}
           	}
           	if(!topic_number){
           		 printf("Invalid topic\n");
           	}

        }
        else if (!strcmp(command, "ts")){
            sscanf(inputptr, "%s", argbuffer);
            topic_number = atoi(argbuffer);

            if (topic_number<=0 || topic_number > topic_amount)
            {
                printf("Invalid topic number\n");
                topic_number=0;
            }
            else
                printf("Selected topic: %s (proposed by %d)\n", topic_list[topic_number].name, topic_list[topic_number].id);
        }
        else if (userid !=-1 && (!strcmp(command, "topic_propose") || !strcmp(command, "tp")))
        {
            sscanf(inputptr, "%s", argbuffer);

            sprintf(message, "PTP %d %s\n", userid, argbuffer);

            if ((n = sendudp(udpfd, &resudp, message, response)) == -1)
                exit(1);
            response[n] = '\0';

            if (!strcmp(response, "PTR NOK\n"))
                printf("Failed to propose topic - check if topic name is alphanumeric and if it's at most 10 characters long\n");
            else if (!strcmp(response, "PTR DUP\n"))
                printf("Failed to propose topic - topic already exists\n");
            else if (!strcmp(response, "PTR FUL\n"))
                printf("Failed to propose topic - topic list full\n");
            else
            {
                printf("Topic proposed successfully\n");
                topic_amount++;
                strcpy(topic_list[topic_amount].name, argbuffer);
                topic_list[topic_amount].id = userid;
                topic_number = topic_amount;
            }
        }
        else if (!strcmp(command, "question_list") || !strcmp(command, "ql"))
        {
            int n, question_amount, question_id, question_na;
            char question_name[16];
            char delim[3] = ": ";

            if (!topic_number)
            {
                printf("No topic selected\n");
                continue;
            }

            sprintf(message, "LQU %s\n", topic_list[topic_number].name);
            if ((n = sendudp(udpfd, &resudp, message, response)) == -1)
                exit(1);
            response[n] = '\0';

            strtok(response, delim);
            question_amount = atoi(strtok(NULL, delim));

            printf("%d questions available for topic %s:\n", question_amount, topic_list[topic_number].name);
            for (int i = 1; i <= question_amount; i++)
            {
                strcpy(question_name, strtok(NULL, delim));
                question_id = atoi(strtok(NULL, delim));
                question_na = atoi(strtok(NULL, delim));
                printf("%d - %s (proposed by %d) - %d answers\n", i, question_name, question_id, question_na);
            }
                

        }
        else if (!strcmp(command, "question_get") || !strcmp(command, "qg"))
        {
            //TODO: NOT YET IMPLEMENTED
        }
        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs"))
        {
            //TODO: NOT YET IMPLEMENTED
        }
        else if (!strcmp(command, "answer_submit") || !strcmp(command, "as"))
        {
            //TODO: NOT YET IMPLEMENTED
        }
        else {
            printf("Unknown command\n");
            continue;
        }
        memset(argbuffer, 0, sizeof buffer);
        memset(input, 0, sizeof buffer);
        memset(command, 0, sizeof buffer);
    }

    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(udpfd);

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