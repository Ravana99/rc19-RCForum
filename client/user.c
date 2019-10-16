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
#include <sys/stat.h>
#include <sys/types.h>

#define P_MAX 512

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

void writeTCP(int tcpfd, char *buffer, ssize_t *buffer_length)
{
    ssize_t nleft, n;
    char *ptr;
    nleft = getMin(1024, *buffer_length) * sizeof(char);
    //printf("NLEFT %zu\n%s\n\n", nleft, message);
    ptr = buffer;
    while (nleft > 0)
    {
        if ((n = write(tcpfd, ptr, nleft)) <= 0)
        {
            write(1, "WERROR\n", 7);
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        nleft -= n;
        *buffer_length -= n;
        ptr += n;
    }
}

void readTCP(int tcpfd, char *buffer, char terminalChar)
{
    ssize_t n;
    char *ptr;
    ptr = buffer;
    do
    {
        if ((n = read(tcpfd, ptr, 1)) == -1)
        {
            write(1, "RERROR\n", 7);
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        ptr += n;
    } while (*(ptr - n) != terminalChar);
    //ptr--;
    *ptr = '\0';
}

void readTCPFull(int fd, char *buffer, long n)
{
    int nr;
    char *ptr;
    ptr = buffer;
    while (n > 0)
    {
        if ((nr = read(fd, ptr, n)) <= 0)
            exit(1);
        n -= nr;
        ptr += nr;
    }
}

void writeTCPFull(int fd, char *str, long n)
{
    int nw;
    char *ptr = &str[0];
    while (n > 0)
    {
        if ((nw = write(fd, ptr, n)) <= 0)
            exit(1);
        n -= nw;
        ptr += nw;
    }
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
                  int active_topic_number, int number_of_topics, Topic active_topic)
{
    int number_of_questions, question_id, question_na, i;
    char question_name[16];
    char delim[3] = ": ";

    if (!active_topic_number)
        printf("No topic selected\n");
    else
    {
        sprintf(message, "LQU %s\n", active_topic.name);
        sendUDP(udpfd, &resudp, message, response);

        strtok(response, delim);
        number_of_questions = atoi(strtok(NULL, delim));

        printf("%d questions available for topic %s:\n", number_of_questions, active_topic.name);
        for (i = 1; i <= number_of_questions; i++)
        {
            strcpy(question_name, strtok(NULL, delim));
            question_id = atoi(strtok(NULL, delim));
            question_na = atoi(strtok(NULL, delim));
            printf("%d - %s (proposed by %d) - %d answers\n", i, question_name, question_id, question_na);
        }
    }
}
void questionGet(int tcpfd, int udpfd, struct addrinfo *resudp, char *inputptr,
                 char *message, char *response, struct addrinfo *restcp, Topic active_topic, bool selectByNumber)
{
    ssize_t message_length;
    FILE *fp;
    char question[11];
    int i, numOfAnswers;
    char qUserID[6], qsize[128], qIMG[2], qiext[4], qisize[128], garbage[10];
    char *qdata, *qidata, *adata, *aidata, pathname[P_MAX], filename[FILENAME_MAX];
    char qCommand[4], aNumber[4], N[3]; //aNumber[4] WHY doesnt 3 work???
    char aUserID[6], asize[128], aIMG[2], aiext[4], aisize[128];

    if (selectByNumber)
    {
        char question_number[6];
        char delim[3] = ": ";
        int number_of_questions, i;
        sscanf(inputptr, "%s", question_number);

        sprintf(message, "LQU %s\n", active_topic.name);
        sendUDP(udpfd, &resudp, message, response);

        strtok(response, delim);
        number_of_questions = atoi(strtok(NULL, delim));

        for (i = 1; i <= number_of_questions; i++)
        {
            strcpy(question, strtok(NULL, delim));
            if (i == atoi(question_number))
                break;
            strtok(NULL, delim);
            strtok(NULL, delim);
        }
    }

    sscanf(inputptr, "%s", question);
    sprintf(message, "GQU %s %s\n", active_topic.name, question);
    message_length = 3 + 1 + strlen(active_topic.name) + 1 + strlen(question) + 1;
    openAndConnectToSocketTCP(restcp, &tcpfd);
    writeTCP(tcpfd, message, &message_length);
    readTCP(tcpfd, qCommand, ' ');
    readTCP(tcpfd, qUserID, ' ');
    readTCP(tcpfd, qsize, ' ');
    qdata = malloc(sizeof(char) * atoi(qsize));

    readTCPFull(tcpfd, qdata, atoi(qsize) / sizeof(char));
    //printf("SIZE%d\n\n", atoi(qsize));
    readTCP(tcpfd, garbage, ' '); // discard space after \n
    readTCP(tcpfd, qIMG, ' ');
    //printf("QIMAGE:%s\n", qIMG);

    //mkdir("client/topics", 0777);
    sprintf(pathname, "client/%s", active_topic.name);
    if (mkdir(pathname, 0777) == -1) // Enables R/W for all users
        printf("%s\n", strerror(errno));

    sprintf(filename, "client/%s/%s.txt", active_topic.name, question);
    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error creating question file!\n\n");
        return;
    }
    fwrite(qdata, sizeof(char), atoi(qsize) / sizeof(char), fp);
    fclose(fp);

    if (atoi(qIMG))
    {
        readTCP(tcpfd, qiext, ' ');
        readTCP(tcpfd, qisize, ' ');
        qidata = malloc(sizeof(char) * atoi(qisize));
        readTCPFull(tcpfd, qidata, atoi(qisize) / sizeof(char));
        readTCP(tcpfd, garbage, '\n'); // discard space after \n

        sprintf(filename, "client/%s/%s.%s", active_topic.name, question, qiext);
        printf("%s %s %s\n", active_topic.name, question, qiext);
        fp = fopen(filename, "wb");
        if (fp == NULL)
        {
            printf("Error creating qImage file!\n\n");
            return;
        }
        fwrite(qidata, sizeof(char), atoi(qisize) / sizeof(char), fp);
        fclose(fp);
        //printf("%s %s %s %s %s %s %s %s\n", qCommand, qUserID, qsize, qdata, qIMG, qiext, qisize, qidata);
    }
    else
    {
        //printf("%s %s %s %s %s\n", qCommand, qUserID, qsize, qdata, qIMG);
    }

    readTCP(tcpfd, N, ' ');
    //printf("%s\n", N);
    numOfAnswers = atoi(N);
    for (i = 0; i < numOfAnswers; i++)
    {
        readTCP(tcpfd, aNumber, ' ');
        aNumber[2] = '\0'; //FIX ON aNumber
        readTCP(tcpfd, aUserID, ' ');
        readTCP(tcpfd, asize, ' ');
        adata = malloc(sizeof(char) * atoi(asize));
        readTCPFull(tcpfd, adata, atoi(asize) / sizeof(char));
        readTCP(tcpfd, garbage, ' '); // discard space after \n
        readTCP(tcpfd, aIMG, ' ');

        sprintf(filename, "client/%s/%s_%s.txt", active_topic.name, question, aNumber);
        fp = fopen(filename, "w");
        if (fp == NULL)
        {
            printf("Error creating answer file!\n\n");
            return;
        }
        fwrite(adata, sizeof(char), atoi(asize) / sizeof(char), fp);
        fclose(fp);

        if (atoi(aIMG))
        {
            readTCP(tcpfd, aiext, ' ');
            readTCP(tcpfd, aisize, ' ');
            aidata = malloc(sizeof(char) * atoi(aisize));
            readTCPFull(tcpfd, aidata, atoi(aisize) / sizeof(char));

            sprintf(filename, "client/%s/%s_%s.%s", active_topic.name, question, aNumber, aiext);
            fp = fopen(filename, "wb");
            if (fp == NULL)
            {
                printf("Error creating aImage file!\n\n");
                return;
            }
            fwrite(aidata, sizeof(char), atoi(aisize) / sizeof(char), fp);
            fclose(fp);
            //  printf("%s %s %s %s %s %s %s %s\n", aNumber, aUserID, asize, adata, aIMG, aiext, aisize, aidata);
        }
        else
        {
            //printf("%s %s %s %s %s\n", aNumber, aUserID, asize, adata, aIMG);
        }
    }
    printf("FINISHED GETTING FILES\n");
    close(tcpfd);
    //free(qdata);
    //free(qidata);
    //free(adata);
    //free(aidata);
}
void questionSubmit(int tcpfd, char *inputptr, char *response, struct addrinfo *restcp, int userid, Topic active_topic, char *active_question)
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
    readTCP(tcpfd, response, '\n');
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
    readTCP(tcpfd, response, '\n');
    printf("%s\n", response);

    close(tcpfd);
    free(message);
}

void quit(struct addrinfo *restcp, struct addrinfo *resudp, int udpfd, char *message)
{
    //free(message);
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
                         active_topic_number, number_of_topics, topic_list[active_topic_number]);

        else if (!strcmp(command, "question_get"))
            questionGet(tcpfd, udpfd, resudp, inputptr, message, response, restcp, topic_list[active_topic_number], false);
        else if (!strcmp(command, "qg"))
            questionGet(tcpfd, udpfd, resudp, inputptr, message, response, restcp, topic_list[active_topic_number], true);

        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs"))
            questionSubmit(tcpfd, inputptr, response, restcp, userid, topic_list[active_topic_number], active_question);

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