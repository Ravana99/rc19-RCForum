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

#define min(A, B) ((A) >= (B) ? (B) : (A))
#define BUFF_MAX 2048
#define P_MAX 512
#define F_MAX 64
#define ID_MAX 8
#define EXT_MAX 8

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
    else if (argc == 3)
    {
        if (!strcmp(argv[1], "-n"))
        {
            strcpy(hostname, argv[2]);
            strcpy(port, "58039");
        }
        else if (!strcmp(argv[1], "-p"))
        {
            strcpy(hostname, "localhost");
            strcpy(port, argv[2]);
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

void readUntilChar(int tcpfd, char *buffer, char terminalChar)
{
    ssize_t n;
    char *ptr;
    ptr = buffer;
    do
    {
        if ((n = read(tcpfd, ptr, 1)) == -1)
        {
            printf("Error reading...\n");
            exit(EXIT_FAILURE);
        }
        ptr += n;
    } while (*(ptr - n) != terminalChar);
    *ptr = '\0';
}

void readMax1024(int fd, char *buffer, long *n)
{
    long int nleft, nr;
    char *ptr;
    nleft = min(1024, *n) * sizeof(char);
    ptr = buffer;
    while (nleft > 0)
    {
        if ((nr = read(fd, ptr, *n)) <= 0)
        {
            printf("Error reading...\n");
            exit(EXIT_FAILURE);
        }
        nleft -= nr;
        *n -= nr;
        ptr += nr;
    }
    *ptr = '\0';
}

void writeMax1024(int tcpfd, char *buffer, ssize_t *buffer_length)
{
    ssize_t nleft, n;
    char *ptr;
    nleft = min(1024, *buffer_length) * sizeof(char);
    //printf("NLEFT %zu\n%s\n\n", nleft, message);
    ptr = buffer;
    while (nleft > 0)
    {
        if ((n = write(tcpfd, ptr, nleft)) <= 0)
        {
            printf("Error writing...\n");
            exit(EXIT_FAILURE);
        }
        nleft -= n;
        *buffer_length -= n;
        ptr += n;
    }
}

/////////////
//COMMANDS///
/////////////

void registerUser(int *userid, char *inputptr, char *message, char *response, int udpfd, struct addrinfo *resudp)
{
    char newID[ID_MAX];
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
    if (!strcmp(response, "LTR 0\n"))
    {
        printf("No topics are yet available\n\n");
        return;
    }
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
    if (number_of_topics == 0)
    {
        printf("Topic list not found\n");
    }
    else if (selectByNumber)
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
    {
        printf("Please select a topic first\n\n");
        return;
    }

    sprintf(message, "LQU %s\n", active_topic.name);
    sendUDP(udpfd, &resudp, message, response);
    if (!strcmp(response, "LTR 0\n"))
    {
        printf("No questions are yet available for this topic\n\n");
        return;
    }
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

void questionGet(int tcpfd, int udpfd, struct addrinfo *resudp, char *inputptr, char *message,
                 char *response, struct addrinfo *restcp, Topic active_topic, char *active_question, bool selectByNumber)
{
    ssize_t message_length;
    FILE *fp;
    char question[11];
    int i, numOfAnswers;
    long int size;
    char qUserID[6], qsize[128], qIMG[4], qiext[4], qisize[128], garbage[10], pathname[P_MAX], filename[FILENAME_MAX];
    char *data;
    char qCommand[4], aNumber[4], N[3]; //aNumber[4] WHY doesnt 3 work???
    char aUserID[6], asize[128], aIMG[4], aiext[8], aisize[128];

    memset(question, 0, sizeof(char) * 11);
    question[10] = '\0';
    data = malloc(sizeof(char) * 1024);

    if (selectByNumber)
    {
        int question_number, n;
        char delim[3] = ": ";
        int number_of_questions, i;
        sprintf(message, "LQU %s\n", active_topic.name);
        sendUDP(udpfd, &resudp, message, response);

        strtok(response, delim);
        number_of_questions = atoi(strtok(NULL, delim));
        n = sscanf(inputptr, "%d", &question_number);
        if (n == 0 || n == EOF)
        {
            strcpy(question, "$");
        }
        for (i = 1; i <= number_of_questions; i++)
        {
            if (i == question_number)
            {
                strcpy(question, strtok(NULL, delim));
                break;
            }
            else
            {
                strtok(NULL, delim);
            }
            strtok(NULL, delim);
            strtok(NULL, delim);
        }
    }

    else
        sscanf(inputptr, "%s", question);

    sprintf(message, "GQU %s %s\n", active_topic.name, question);
    message_length = 3 + 1 + strlen(active_topic.name) + 1 + strlen(question) + 1;
    openAndConnectToSocketTCP(restcp, &tcpfd);
    //printf("%s\n", message);
    writeMax1024(tcpfd, message, &message_length);
    readUntilChar(tcpfd, qCommand, ' ');
    printf("QUESTION:%s\n", qCommand);

    size = 3;
    readMax1024(tcpfd, qUserID, &size);
    qUserID[3] = '\0';
    printf("USERID:%s", qUserID);
    if (!strcmp(qUserID, "ERR"))
    {
        printf("QGR ERR\n\n");
        return;
    }
    else if (!strcmp(qUserID, "EOF"))
    {
        printf("QGR EOF\n\n");
        return;
    }

    size = 2;
    readMax1024(tcpfd, qUserID + 3, &size);
    qUserID[5] = '\0';
    printf("USERID:%s", qUserID);
    readUntilChar(tcpfd, qsize, ' ');

    sprintf(pathname, "client/%s", active_topic.name);
    if (mkdir(pathname, 0777) == -1)
    { // Enables R/W for all users
        if (errno != EEXIST)
        {
            printf("%s\n", strerror(errno));
            return;
        }
    }
    //set R/W permissions to everyone
    if (chmod(pathname, 0777) == -1)
    {
        printf("%s\n", strerror(errno));
        return;
    }

    sprintf(filename, "%s/%s.txt", pathname, question);

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error creating question file!\n\n");
        return;
    }

    size = atol(qsize);
    while (size > 0)
    {
        readMax1024(tcpfd, data, &size);
        fwrite(data, sizeof(char), min(1024, size), fp);
    }
    fclose(fp);

    printf("QDATA:%s\n", data);
    readUntilChar(tcpfd, garbage, ' '); // discard space after \n
    readUntilChar(tcpfd, qIMG, ' ');
    printf("HERE:%s\n", qCommand);
    //mkdir("client/topics", 0777);

    if (atoi(qIMG))
    {
        readUntilChar(tcpfd, qiext, ' ');
        qiext[3] = '\0';
        readUntilChar(tcpfd, qisize, ' ');
        //printf("HERE:%s\n", qCommand);
        sprintf(filename, "client/%s/%s.%s", active_topic.name, question, qiext);

        fp = fopen(filename, "wb");
        if (fp == NULL)
        {
            printf("Error creating question image file!\n\n");
            printf("%s\n", strerror(errno));
            return;
        }
        size = atol(qisize);
        while (size > 0)
        {
            readMax1024(tcpfd, data, &size);
            fwrite(data, sizeof(char), min(1024, size), fp);
        }
        fclose(fp);
        //printf("HERE:%s\n", qCommand);
        readUntilChar(tcpfd, garbage, ' ');
    }
    readUntilChar(tcpfd, N, ' ');
    printf("H22ERE:%s\n", qIMG);
    printf("H22ERE:%s\n", N);
    numOfAnswers = atoi(N);
    for (i = 0; i < numOfAnswers; i++)
    {
        readUntilChar(tcpfd, aNumber, ' ');
        aNumber[2] = '\0'; //FIX ON aNumber
        readUntilChar(tcpfd, aUserID, ' ');
        readUntilChar(tcpfd, asize, ' ');

        if (fp == NULL)
        {
            printf("Error creating answer file!\n\n");
            return;
        }
        size = atol(asize);
        while (size > 0)
        {
            readMax1024(tcpfd, data, &size);
            fwrite(data, sizeof(char), min(1024, size), fp);
        }
        fclose(fp);

        readUntilChar(tcpfd, garbage, ' '); // discard space after \n
        if (i == numOfAnswers - 1)
        {
            size = 1;
            readMax1024(tcpfd, aIMG, &size);
            if (aIMG[0] == '1')
                readUntilChar(tcpfd, garbage, ' ');
        }
        else
            readUntilChar(tcpfd, aIMG, ' ');

        sprintf(filename, "client/%s/%s_%s.txt", active_topic.name, question, aNumber);
        fp = fopen(filename, "w");

        if (atoi(aIMG))
        {
            readUntilChar(tcpfd, aiext, ' ');
            aiext[3] = '\0';
            readUntilChar(tcpfd, aisize, ' ');

            if (fp == NULL)
            {
                printf("Error creating answer image file!\n\n");
                return;
            }
            size = atol(asize);
            while (size > 0)
            {
                readMax1024(tcpfd, data, &size);
                fwrite(data, sizeof(char), min(1024, size), fp);
            }
            fclose(fp);

            sprintf(filename, "client/%s/%s_%s.%s", active_topic.name, question, aNumber, aiext);

            if (i == numOfAnswers - 1)
                readUntilChar(tcpfd, garbage, '\n');
            else
                readUntilChar(tcpfd, garbage, ' ');
        }
    }

    printf("Question:%s sucessfully downloaded\n\n", question);
    strcpy(active_question, question);
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
    char question[16], filename[NAME_MAX + 3], imagefilename[NAME_MAX + 3],
        qdata[2048], iext[8], *message, imgBuffer[1024];
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
    qdata[qsize] = '\0';
    //printf("%s\n", qdata);

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
        message = malloc(sizeof(char) * (message_length + 1));
        sprintf(message, "QUS %d %s %s %ld %s 1 %s %ld ",
                userid, active_topic.name, question, qsize, qdata, iext, isize);

        //printf("%s\n", message);
        writeMax1024(tcpfd, message, &message_length); //write of everything, except image content
        while (isize > 0)                              // write of image content, 1024 characters in each iteration
        {
            n = read(fileno(fp), imgBuffer, sizeof(char) * 1024);
            isize -= n;
            writeMax1024(tcpfd, imgBuffer, &n);
        }
        isize = 1;
        writeMax1024(tcpfd, "\n", &isize);

        fclose(fp);
    }
    else
    {
        message = malloc(sizeof(char) * (message_length + 1));
        sprintf(message, "QUS %d %s %s %ld %s 0\n", userid, active_topic.name, question, qsize, qdata);
        writeMax1024(tcpfd, message, &message_length);
    }

    readUntilChar(tcpfd, response, '\n');
    if (!strcmp(response, "QUR NOK\n"))
        printf("Failed to submit question - check if question name is alphanumeric and if it's at most 10 characters long\n\n");
    else if (!strcmp(response, "QUR DUP\n"))
        printf("Failed to submit question - question already exists\n\n");
    else if (!strcmp(response, "QUR FUL\n"))
        printf("Failed to submit question - question list full\n\n");
    else
        printf("Question submited sucessfully\n\n");
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
    adata[asize] = '\0';
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
        message = malloc(sizeof(char) * (message_length + 1));
        sprintf(message, "ANS %d %s %s %ld %s 1 %s %ld ",
                userid, active_topic.name, active_question, asize, adata, iext, isize);

        writeMax1024(tcpfd, message, &message_length); //write of everything, except image content
        while (isize > 0)                              // write of image content, 1024 characters in each iteration
        {
            n = read(fileno(fp), imgBuffer, sizeof(char) * 1024);
            isize -= n;
            writeMax1024(tcpfd, imgBuffer, &n);
        }
        isize = 1;
        writeMax1024(tcpfd, "\n", &isize);

        fclose(fp);
    }
    else
    {
        message = malloc(sizeof(char) * (message_length + 1));
        sprintf(message, "ANS %d %s %s %ld %s 0\n", userid, active_topic.name, active_question, asize, adata);
        writeMax1024(tcpfd, message, &message_length);
    }

    readUntilChar(tcpfd, response, '\n');

    if (!strcmp(response, "AMR NOK\n"))
        printf("Failed to submit answer\n\n");
    else if (!strcmp(response, "ANR FUL\n"))
        printf("Failed to submit question - answer list full\n\n");
    else
        printf("Answer submited sucessfully\n\n");
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
    //TROCAR ACTIVE_TOPIC_NUMBER POR ACTIVE_TOPIC
    Topic topic_list[99 + 1]; // active_topic; // First entry is left empty to facilitate indexing
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;

    int tcpfd = 0, udpfd = 0, active_topic_number = 0, number_of_topics = 0, userid = 0;
    struct sigaction act;

    char port[16], hostname[128], buffer[128], input[128], command[128], response[2048], active_question[16];
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

        else if (!strcmp(command, "topic_propose") || !strcmp(command, "tp"))
        {
            if (userid == 0)
                printf("Please register your userID first\n");
            else
                topicPropose(inputptr, message, response, udpfd, resudp,
                             userid, &active_topic_number, &number_of_topics, topic_list, buffer);
        }

        else if (!strcmp(command, "question_list") || !strcmp(command, "ql"))
            questionList(message, response, udpfd, resudp,
                         active_topic_number, number_of_topics, topic_list[active_topic_number]);

        else if (!strcmp(command, "question_get"))
            questionGet(tcpfd, udpfd, resudp, inputptr, message, response, restcp, topic_list[active_topic_number], active_question, false);
        else if (!strcmp(command, "qg"))
            questionGet(tcpfd, udpfd, resudp, inputptr, message, response, restcp, topic_list[active_topic_number], active_question, true);

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