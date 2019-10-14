/* Group 39 - Joao Fonseca 89476, Tiago Pires 89544, Tomas Lopes 89552 */

//send only topic instead of sending topic number and topic list?

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
#include <limits.h>

struct answer
{
    char name[16];
} typedef Answer;
struct topic
{
    char name[16];
    int id;
    Answer answers[99];
    //guardar as respostas aqui ou fzr pedido udp?
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

void getImageExtension(char *imagefilename, char *iext)
{
    int i, j = 0, imagefilename_length = strlen(imagefilename);
    for (i = imagefilename_length - 3; i < imagefilename_length; i++)
    {
        iext[j++] = imagefilename[i];
    }
    iext[j] = '\0';
}

int getMin(long int x, long int y)
{
    return (x < y) ? x : y;
}

int getNumberOfDigits(int number)
{
    int count = 0;
    while (number != 0)
    {
        number /= 10;
        ++count;
    }
    return count;
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
        exit(EXIT_FAILURE);
    response[n] = '\0'; // Appends a '\0' to the response so it can be used in strcmp
    return n;
}

void openAndConnectToSocketTCP(struct addrinfo *restcp, int *tcpfd)
{

    if ((*tcpfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
    {
        printf("Socket error\n");
        exit(EXIT_FAILURE);
    }
    if (connect(*tcpfd, restcp->ai_addr, restcp->ai_addrlen) == -1)
    {
        printf("Connection error\n");
        exit(1);
    }
}

void writeTCP(int tcpfd, char *message, ssize_t *message_length)
{
    ssize_t nleft, n;
    char *ptr;
    nleft = getMin(1024, *message_length) * sizeof(char);
    //printf("NLEFT %zu\n%s\n\n", nleft, message);
    ptr = message;
    while (nleft > 0)
    {
        if ((n = write(tcpfd, ptr, nleft)) <= 0)
        {
            write(1, "WERROR\n", 7);
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        nleft -= n;
        *message_length -= n;
        ptr += n;
    }
}

void readTCP(int tcpfd, char *response)
{
    ssize_t n;
    // char *ptr;
    //  ptr = response;
    do
    {
        if ((n = read(tcpfd, response, 1)) == -1)
        {
            write(1, "RERROR\n", 7);
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
            break;
        response += n;
    } while (*(response - n) != '\n');
    //close(tcpfd);
}

/////////////
//COMMANDS///
/////////////

void registerUser(int *userid, char *inputptr, char *message, char *response, int udpfd, struct addrinfo *resudp)
{
    char newID[5];
    sscanf(inputptr, "%s", newID);
    sprintf(message, "REG %s\n", newID);
    sendUDP(udpfd, &resudp, message, response);

    if (!strcmp(response, "RGR OK\n"))
    {
        *userid = atoi(newID);
        printf("User \"%d\" registered\n\n", *userid);
    }
    else
        printf("Registration failed\n\n");
}
void topicList(char *message, char *response, int udpfd, struct addrinfo *resudp, int *number_of_topics, Topic *topic_list)
{
    char delim[3] = ": ";
    int n, i;

    strcpy(message, "LTP\n");
    sendUDP(udpfd, &resudp, message, response);
    strtok(response, delim);
    n = atoi(strtok(NULL, delim));
    *number_of_topics = n;
    printf("Available topics:\n");
    for (i = 1; i <= n; i++)
    {
        strcpy(topic_list[i].name, strtok(NULL, delim));
        topic_list[i].id = atoi(strtok(NULL, delim));
        printf("Topic %d - %s (proposed by %d)\n", i, topic_list[i].name, topic_list[i].id);
    }
    printf("\n");
}
void topicSelect(char *inputptr, int *active_topic_number, int number_of_topics, Topic *topic_list, bool selectByNumber, char *buffer)
{
    int tmp_topic_number = 0, i;
    if (selectByNumber)
    {
        char small_buffer[5];

        sscanf(inputptr, "%s", small_buffer);
        tmp_topic_number = atoi(small_buffer);

        if (tmp_topic_number <= 0 || tmp_topic_number > number_of_topics)
            printf("Invalid topic number\n\n");
        else
        {
            *active_topic_number = tmp_topic_number;
            printf("Selected topic: %s (proposed by %d)\n\n", topic_list[*active_topic_number].name, topic_list[*active_topic_number].id);
        }
    }
    else
    {
        sscanf(inputptr, "%s", buffer);
        for (i = 1; i <= number_of_topics; i++)
            if (!strcmp(buffer, topic_list[i].name))
            {
                printf("Selected topic: %s (proposed by %d)\n\n", topic_list[i].name, topic_list[i].id);
                *active_topic_number = tmp_topic_number = i;
                break;
            }
        if (!tmp_topic_number)
            printf("Invalid topic name\n\n");
    }
}
void topicPropose(char *inputptr, char *message, char *response, int udpfd, struct addrinfo *resudp,
                  int userid, int *active_topic_number, int *number_of_topics, Topic *topic_list, char *buffer)
{
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
        (*number_of_topics)++;
        strcpy(topic_list[*number_of_topics].name, buffer);
        topic_list[*number_of_topics].id = userid;
        *active_topic_number = *number_of_topics;
    }
}
void questionList(char *message, char *response, int udpfd, struct addrinfo *resudp,
                  int active_topic_number, int number_of_topics, Topic *topic_list)
{
    int number_of_questions, question_id, question_na, i;
    char question_name[16];
    char delim[3] = ": ";

    if (!active_topic_number)
        printf("No topic selected\n");
    else
    {
        sprintf(message, "LQU %s\n", topic_list[active_topic_number].name);
        sendUDP(udpfd, &resudp, message, response);

        strtok(response, delim);
        number_of_questions = atoi(strtok(NULL, delim));

        printf("%d questions available for topic %s:\n", number_of_questions, topic_list[active_topic_number].name);
        for (i = 1; i <= number_of_questions; i++)
        {
            strcpy(question_name, strtok(NULL, delim));
            question_id = atoi(strtok(NULL, delim));
            question_na = atoi(strtok(NULL, delim));
            printf("%d - %s (proposed by %d) - %d answers\n", i, question_name, question_id, question_na);
        }
    }
}
void questionGet(char *message, char *response, struct addrinfo *restcp, int active_topic_number, Topic *topic_list, bool selectByNumber)
{
    /*if (selectByNumber)
    { 
        char small_buffer[5];
        sscanf(inputptr, "%s", small_buffer);
        tmp_topic_number = atoi(small_buffer);

        if (tmp_topic_number <= 0 || tmp_topic_number > number_of_topics)
            printf("Invalid topic number\n\n");
        else
        {
            *active_topic_number = tmp_topic_number;
            printf("Selected topic: %s (proposed by %d)\n\n", topic_list[*active_topic_number].name, topic_list[*active_topic_number].id);
        }
    }
    else
    {
        sscanf(inputptr, "%s", buffer);
        for (int i = 1; i <= number_of_topics; i++)
            if (!strcmp(buffer, topic_list[i].name))
            {
                printf("Selected topic: %s (proposed by %d)\n\n", topic_list[i].name, topic_list[i].id);
                *active_topic_number = tmp_topic_number = i;
                break;
            }
        if (!tmp_topic_number)
            printf("Invalid topic name\n\n");
    }*/
}
void questionSubmit(int tcpfd, char *inputptr, char *response, struct addrinfo *restcp, int userid, int active_topic_number, Topic active_topic, char *active_question)
{
    FILE *fp;
    long int message_length;
    char question[11], filename[NAME_MAX + 3], imagefilename[NAME_MAX + 3],
        qdata[2048], iext[4], *message, imgBuffer[1024];
    ssize_t qsize = 0, isize = 0, n;

    imagefilename[0] = '\0';
    sscanf(inputptr, "%s %s %s", question, filename, imagefilename);
    strcpy(active_question, question);

    strcat(filename, ".txt");
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Question File Not Found!\n\n");
        return;
    }
    fseek(fp, 0L, SEEK_END);
    qsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fread(qdata, sizeof(char) * qsize, 1, fp);
    fclose(fp);

    message_length = 3 + 1 + 5 + 1 + strlen(active_topic.name) +
                     1 + strlen(question) + 1 + getNumberOfDigits(qsize) + 1 +
                     strlen(qdata) + 1 + 1 + 1;

    openAndConnectToSocketTCP(restcp, &tcpfd);

    if (imagefilename[0] != '\0')
    {
        fp = fopen(imagefilename, "rb");
        if (fp == NULL)
        {
            printf("Image File Not Found!\n\n");
            return;
        }

        getImageExtension(imagefilename, iext);

        fseek(fp, 0L, SEEK_END);
        isize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        message_length += 3 + 1 + getNumberOfDigits(isize) + 1;
        message = malloc(sizeof(char) * message_length);
        sprintf(message, "QUS %d %s %s %ld %s 1 %s %ld ",
                userid, active_topic.name, question, qsize, qdata, iext, isize);

        writeTCP(tcpfd, message, &message_length); //write of everything, except image content
        while (isize > 0)                          // write of image content, 1024 characters in each iteration
        {
            n = read(fileno(fp), imgBuffer, sizeof(char) * 1024);
            isize -= n;
            writeTCP(tcpfd, imgBuffer, &n);
        }
        isize = 1;
        writeTCP(tcpfd, "\n", &isize);

        fclose(fp);
    }
    else
    {
        message = malloc(sizeof(char) * message_length);
        sprintf(message, "QUS %d %s %s %ld %s 0\n", userid, active_topic.name, question, qsize, qdata);
        writeTCP(tcpfd, message, &message_length);
    }
    readTCP(tcpfd, response);
    printf("%s\n", response);

    close(tcpfd);
    free(message);
}
void answerSubmit(int tcpfd, char *inputptr, char *response, struct addrinfo *restcp,
                  int userid, Topic active_topic, char *active_question)
{
    FILE *fp;
    long int message_length;
    char filename[NAME_MAX + 3], imagefilename[NAME_MAX + 3],
        adata[2048], iext[4], *message, imgBuffer[1024];
    ssize_t asize = 0, isize = 0, n;

    imagefilename[0] = '\0';
    sscanf(inputptr, "%s %s", filename, imagefilename);

    strcat(filename, ".txt");
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Answer File Not Found!\n\n");
        return;
    }
    fseek(fp, 0L, SEEK_END);
    asize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fread(adata, sizeof(char) * asize, 1, fp);
    fclose(fp);

    message_length = 3 + 1 + 5 + 1 + strlen(active_topic.name) +
                     1 + strlen(active_question) + 1 + getNumberOfDigits(asize) + 1 +
                     strlen(adata) + 1 + 1 + 1;

    openAndConnectToSocketTCP(restcp, &tcpfd);

    if (imagefilename[0] != '\0')
    {
        fp = fopen(imagefilename, "rb");
        if (fp == NULL)
        {
            printf("Image File Not Found!\n\n");
            return;
        }

        getImageExtension(imagefilename, iext);

        fseek(fp, 0L, SEEK_END);
        isize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        message_length += 3 + 1 + getNumberOfDigits(isize) + 1;
        message = malloc(sizeof(char) * message_length);
        sprintf(message, "ANS %d %s %s %ld %s 1 %s %ld ",
                userid, active_topic.name, active_question, asize, adata, iext, isize);

        writeTCP(tcpfd, message, &message_length); //write of everything, except image content
        while (isize > 0)                          // write of image content, 1024 characters in each iteration
        {
            n = read(fileno(fp), imgBuffer, sizeof(char) * 1024);
            isize -= n;
            writeTCP(tcpfd, imgBuffer, &n);
        }
        isize = 1;
        writeTCP(tcpfd, "\n", &isize);

        fclose(fp);
    }
    else
    {
        message = malloc(sizeof(char) * message_length);
        sprintf(message, "ANS %d %s %s %ld %s 0\n", userid, active_topic.name, active_question, asize, adata);
        writeTCP(tcpfd, message, &message_length);
    }
    readTCP(tcpfd, response);
    printf("%s\n", response);

    close(tcpfd);
    free(message);
}

void quit(struct addrinfo *restcp, struct addrinfo *resudp, int udpfd, char *message)
{
    free(message);
    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(udpfd);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    Topic topic_list[99 + 1]; // First entry is left empty to facilitate indexing
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;

    int tcpfd = 0, udpfd = 0, active_topic_number = 0, number_of_topics = 0, userid = 0;
    struct sigaction act;

    char port[16], hostname[128], buffer[128], input[128], command[128], response[2048], active_question[11];
    char *inputptr, *message;

    //memset(topic_list, 0, sizeof topic_list);
    message = malloc(sizeof(char) * 2048);
    setHostNameAndPort(argc, argv, hostname, port);

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

    if (getaddrinfo(hostname, port, &hintstcp, &restcp) != 0)
        exit(EXIT_FAILURE);
    if (getaddrinfo(hostname, port, &hintsudp, &resudp) != 0)
        exit(EXIT_FAILURE);

    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1)
        exit(EXIT_FAILURE);

    while (1)
    {

        //printf("Number of topics %d\nActive Topic %s\nUserID %d\n", number_of_topics, topic_list[active_topic_number].name, userid);
        fgets(input, 2048, stdin);
        if (!strcmp(input, "\n"))
            continue;
        sscanf(input, "%s", command);
        inputptr = input;
        inputptr += strlen(command) + 1; // Points to arguments of the command

        if (!strcmp(command, "exit"))
            quit(restcp, resudp, udpfd, message);
        else if (!strcmp(command, "register") || !strcmp(command, "reg"))
            registerUser(&userid, inputptr, message, response, udpfd, resudp);

        else if (!strcmp(command, "topic_list") || !strcmp(command, "tl"))
            topicList(message, response, udpfd, resudp, &number_of_topics, topic_list);

        else if (!strcmp(command, "topic_select"))
            topicSelect(inputptr, &active_topic_number, number_of_topics, topic_list, false, buffer);
        else if (!strcmp(command, "ts"))
            topicSelect(inputptr, &active_topic_number, number_of_topics, topic_list, true, buffer);

        else if (userid != 0 && (!strcmp(command, "topic_propose") || !strcmp(command, "tp")))
            topicPropose(inputptr, message, response, udpfd, resudp,
                         userid, &active_topic_number, &number_of_topics, topic_list, buffer);

        else if (!strcmp(command, "question_list") || !strcmp(command, "ql"))
            questionList(message, response, udpfd, resudp,
                         active_topic_number, number_of_topics, topic_list);

        else if (!strcmp(command, "question_get"))
            questionGet(message, response, restcp, active_topic_number, topic_list, false);
        else if (!strcmp(command, "qg"))
            questionGet(message, response, restcp, active_topic_number, topic_list, true);

        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs"))
            questionSubmit(tcpfd, inputptr, response, restcp, userid, active_topic_number, topic_list[active_topic_number], active_question);

        else if (!strcmp(command, "answer_submit") || !strcmp(command, "as"))
            answerSubmit(tcpfd, inputptr, response, restcp, userid, topic_list[active_topic_number], active_question);

        else
            printf("Unknown command\n\n");

        free(message);
        message = malloc(sizeof(char) * 2048);
        memset(response, 0, sizeof(char) * 2048);
        memset(buffer, 0, sizeof buffer);
        memset(input, 0, sizeof input);
        memset(command, 0, sizeof command);
    }
}