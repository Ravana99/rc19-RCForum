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
#include <stdbool.h>

struct topic
{
    char name[16];
    int id;
} typedef Topic;

void setHostNameAndPort(int argc, char **argv, char *hostname, char *port)
{
    if (argc == 1)
    {
        strcpy(hostname, "localhost");
        strcpy(port, "58039");
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
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Invalid command line arguments\n");
        exit(EXIT_FAILURE);
    }
}

int sendUDP(int udpfd, struct addrinfo **resudp, char *message, char *response)
{
    struct sockaddr_in addr;
    socklen_t addrlen;
    ssize_t n;
    if (sendto(udpfd, message, strlen(message), 0, (*resudp)->ai_addr, (*resudp)->ai_addrlen) == -1)
        return -1;
    addrlen = sizeof(addr);
    n = recvfrom(udpfd, response, 2048, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1)
    {
        exit(EXIT_FAILURE);
    };
    response[n] = '\0'; // Appends a '\0' to the response so it can be used in strcmp
    return n;
}

/////////////
//COMMANDS///
/////////////

void registerUser(int *userid, char *inputptr, char *message, char *response, int udpfd, struct addrinfo *resudp)
{
    char *newID = malloc(sizeof(char) * 5);
    sscanf(inputptr, "%s", newID);
    sprintf(message, "REG %s\n", newID);
    sendUDP(udpfd, &resudp, message, response);

    if (!strcmp(response, "RGR OK\n"))
    {
        *userid = atoi(newID);
        printf("User \"%d\" registered\n", *userid);
    }
    else
    {
        printf("Registration failed\n");
    }
    free(newID);
}
void topicList(char *message, char *response, int udpfd, struct addrinfo *resudp, int *topic_number, int *number_of_topics, Topic *topic_list)
{
    char old_topic[16];
    char delim[3] = ": ";
    int n;

    strcpy(old_topic, topic_list[*topic_number].name);
    strcpy(message, "LTP\n");
    sendUDP(udpfd, &resudp, message, response);
    strtok(response, delim);
    n = atoi(strtok(NULL, delim));
    *number_of_topics = n;
    for (int i = 1; i <= n; i++)
    {
        strcpy(topic_list[i].name, strtok(NULL, delim));
        topic_list[i].id = atoi(strtok(NULL, delim));
        if (!strcmp(old_topic, topic_list[i].name))
            *topic_number = i;
    }
    printf("Available topics:\n");
    for (int i = 1; i <= n; i++)
        printf("Topic %d - %s (proposed by %d)\n", i, topic_list[i].name, topic_list[i].id);
}
void topicSelect(char *inputptr, int topic_number, int number_of_topics, Topic *topic_list, bool selectByNumber)
{
    if (selectByNumber)
    {
        char buffer[5];
        sscanf(inputptr, "%s", buffer);
        topic_number = atoi(buffer);
        printf("%d\n", number_of_topics);
        if (topic_number <= 0 || topic_number > number_of_topics)
        {
            printf("Invalid topic number\n");
            topic_number = 0;
        }
        else
            printf("Selected topic: %s (proposed by %d)\n", topic_list[topic_number].name, topic_list[topic_number].id);
    }
    else
    {
        char topic_name[128];

        sscanf(inputptr, "%s", topic_name);
        topic_number = 0;
        for (int i = 1; i <= number_of_topics; i++)
        {
            if (!strcmp(topic_name, topic_list[i].name))
            {
                printf("Selected topic: %s (proposed by %d)\n", topic_list[i].name, topic_list[i].id);
                topic_number = i;
                break;
            }
        }
        if (!topic_number)
        {
            printf("Invalid topic\n");
        }
    }
}
void topicPropose(char *inputptr, char *message, char *response, int udpfd, struct addrinfo *resudp,
                  int userid, int topic_number, int number_of_topics, Topic *topic_list)
{
    char buffer[128];
    sscanf(inputptr, "%s", buffer);

    sprintf(message, "PTP %d %s\n", userid, buffer);

    sendUDP(udpfd, &resudp, message, response);

    if (!strcmp(response, "PTR NOK\n"))
        printf("Failed to propose topic - check if topic name is alphanumeric and if it's at most 10 characters long\n");
    else if (!strcmp(response, "PTR DUP\n"))
        printf("Failed to propose topic - topic already exists\n");
    else if (!strcmp(response, "PTR FUL\n"))
        printf("Failed to propose topic - topic list full\n");
    else
    {
        printf("Topic proposed successfully\n");
        number_of_topics++;
        strcpy(topic_list[number_of_topics].name, buffer);
        topic_list[number_of_topics].id = userid;
        topic_number = number_of_topics;
    }
}
void questionList(char *message, char *response, int udpfd, struct addrinfo *resudp,
                  int topic_number, int number_of_topics, Topic *topic_list)
{
    int question_amount, question_id, question_na;
    char question_name[16];
    char delim[3] = ": ";

    if (!topic_number)
    {
        printf("No topic selected\n");
    }
    else
    {
        sprintf(message, "LQU %s\n", topic_list[topic_number].name);
        sendUDP(udpfd, &resudp, message, response);

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
}
void questionGet()
{
}
void questionSubmit()
{
}
void answerSubmit()
{
}
void quit(struct addrinfo *restcp, struct addrinfo *resudp, int udpfd)
{
    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(udpfd);
    exit(EXIT_SUCCESS);
}
int main(int argc, char **argv)
{
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    ssize_t n, nbytes, nleft, nwritten, nread;

    int tcpfd, udpfd, errcode, topic_number = 0, number_of_topics = 0, userid = 0;
    struct sigaction act;

    char port[16], hostname[128], buffer[128], received[128], command[128], input[2048];
    char *inputptr, *ptr;
    char *message = malloc(sizeof(char) * 2048);
    char *response = malloc(sizeof(char) * 2048);

    Topic topic_list[99 + 1]; // First entry is left empty to facilitate indexing
    memset(topic_list, 0, sizeof topic_list);

    memset(received, 0, sizeof received);
    memset(&hintstcp, 0, sizeof hintstcp);
    memset(&hintsudp, 0, sizeof hintsudp);
    hintstcp.ai_family = AF_INET;
    hintstcp.ai_socktype = SOCK_STREAM;
    hintstcp.ai_flags = AI_NUMERICSERV;

    hintsudp.ai_family = AF_INET;
    hintsudp.ai_socktype = SOCK_DGRAM;
    hintsudp.ai_flags = AI_NUMERICSERV;

    // Ignoring the SIGPIPE signal
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
        exit(EXIT_FAILURE);

    setHostNameAndPort(argc, argv, hostname, port);

    if (getaddrinfo(hostname, port, &hintstcp, &restcp) != 0)
        exit(EXIT_FAILURE);
    if (getaddrinfo(hostname, port, &hintsudp, &resudp) != 0)
        exit(EXIT_FAILURE);

    if ((tcpfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
        exit(EXIT_FAILURE);
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1)
        exit(EXIT_FAILURE);

    while (1)
    {
        fgets(input, 2048, stdin);
        if (!strcmp(input, "\n"))
            continue;
        sscanf(input, "%s", command);
        inputptr = input;
        inputptr += strlen(command) + 1; // Points to arguments of the command

        if (!strcmp(command, "exit"))
            quit(restcp, resudp, udpfd);
        else if (!strcmp(command, "register") || !strcmp(command, "reg"))
            registerUser(&userid, inputptr, message, response, udpfd, resudp);

        else if (!strcmp(command, "topic_list") || !strcmp(command, "tl"))
            topicList(message, response, udpfd, resudp, &topic_number, &number_of_topics, topic_list);

        else if (!strcmp(command, "topic_select"))
            topicSelect(inputptr, topic_number, number_of_topics, topic_list, false);

        else if (!strcmp(command, "ts"))
            topicSelect(inputptr, topic_number, number_of_topics, topic_list, true);

        else if (userid != 0 && (!strcmp(command, "topic_propose") || !strcmp(command, "tp")))
            topicPropose(inputptr, message, response, udpfd, resudp,
                         userid, topic_number, number_of_topics, topic_list);

        else if (!strcmp(command, "question_list") || !strcmp(command, "ql"))
            questionList(message, response, udpfd, resudp,
                         topic_number, number_of_topics, topic_list);

        else if (!strcmp(command, "question_get") || !strcmp(command, "qg"))
            continue;
        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs"))
            continue;
        else if (!strcmp(command, "answer_submit") || !strcmp(command, "as"))
            continue;
        else
            printf("Unknown command\n");

        memset(message, 0, sizeof(char) * 2048);
        memset(response, 0, sizeof(char) * 2048);
        memset(buffer, 0, sizeof buffer);
        memset(input, 0, sizeof buffer);
        memset(command, 0, sizeof buffer);
    }
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