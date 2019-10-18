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
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        printf("Connection timed out\n");
        return 0;
    }
    else if (n == -1)
    {
        exit(EXIT_FAILURE);
    }
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

    if (connect(*tcpfd, restcp->ai_addr, restcp->ai_addrlen) < 0)
    {
        printf("Connection error\n");
        exit(1);
    }
}

void readUntilChar(int fd, char *buffer, char terminalChar)
{
    ssize_t n;
    char *ptr;
    ptr = buffer;
    do
    {
        if ((n = read(fd, ptr, 1)) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Connection timed out\n");
                return;
            }
            printf("Error reading...\n");
            exit(EXIT_FAILURE);
        }
        ptr += n;
    } while (*(ptr - n) != terminalChar);
    ptr -= n;
    *ptr = '\0';
}

int readMax1024(int fd, char *buffer, long *buffer_length)
{
    long int nleft, n, totalBytesRead;
    char *ptr;
    nleft = min(1024, *buffer_length) * sizeof(char);
    totalBytesRead = nleft;
    ptr = buffer;
    while (nleft > 0)
    {
        if ((n = read(fd, ptr, nleft)) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Connection timed out\n");
                return -1;
            }
            printf("\n%d\n\n", errno);
            printf("Error reading...\n");
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            break;
        }
        nleft -= n;
        *buffer_length -= n;
        ptr += n;
    }
    *ptr = '\0';
    return totalBytesRead;
}

bool isNextCharEmptySpace(int fd, char *buffer)
{
    ssize_t n;
    char *ptr;
    ptr = buffer;
    if ((n = read(fd, ptr, 1)) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("Connection timed out\n");
            return -1;
        }
        printf("Error reading...\n");
        exit(EXIT_FAILURE);
    }
    ptr++;
    *ptr = '\0';
    ptr--;
    if (*(ptr) == ' ')
        return true;
    else
        return false;
}

void writeMax1024(int fd, char *buffer, ssize_t *buffer_length)
{
    ssize_t nleft, n;
    char *ptr;
    nleft = min(1024, *buffer_length) * sizeof(char);
    ptr = buffer;
    while (nleft > 0)
    {
        if ((n = write(fd, ptr, nleft)) <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Connection timed out\n");
                return;
            }
            printf("Error writing...\n");
            printf("%s\n", strerror(errno));
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
    int i, numOfAnswers, bytesRead;
    long int size;
    char qUserID[ID_MAX], qsize[128], qIMG[4], qiext[4], qisize[128], garbage[128], pathname[P_MAX], filename[FILENAME_MAX];
    char *data, *ptrData;
    char qCommand[4], aNumber[4], N[3]; //aNumber[4] WHY doesnt 3 work???
    char aUserID[6], asize[128], aIMG[4], aiext[8], aisize[128];

    memset(question, 0, sizeof(char) * 11);
    question[10] = '\0';
    data = (char*)malloc(sizeof(char) * 1026);

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
    writeMax1024(tcpfd, message, &message_length);

    if (isNextCharEmptySpace(tcpfd, qCommand))
    {
        printf("QGR ERR\n");
        return;
    }
    readUntilChar(tcpfd, qCommand + 1, ' ');

    if (isNextCharEmptySpace(tcpfd, qUserID))
    {
        printf("QGR ERR\n");
        return;
    }
    size = 2;
    readMax1024(tcpfd, qUserID + 1, &size);
    qUserID[3] = '\0';
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

    size = 3;
    readMax1024(tcpfd, qUserID + 3, &size);
    qUserID[5] = '\0';
    if (isNextCharEmptySpace(tcpfd, qsize))
    {
        printf("QGR ERR\n");
        return;
    }
    readUntilChar(tcpfd, qsize + 1, ' ');
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
    if (isNextCharEmptySpace(tcpfd, data))
    {
        printf("QGR ERR\n");
        return;
    }
    ptrData = data + 1;
    size--;
    fwrite(data, sizeof(char), 1, fp);
    while (size > 0)
    {
        bytesRead = readMax1024(tcpfd, ptrData, &size);
        fwrite(ptrData, sizeof(char), bytesRead, fp);
    }
    fclose(fp);

    readUntilChar(tcpfd, garbage, ' '); // discard space after \n
    if (isNextCharEmptySpace(tcpfd, qIMG))
    {
        printf("QGR ERR\n");
        return;
    }
    readUntilChar(tcpfd, qIMG + 1, ' ');
    if (atoi(qIMG))
    {
        if (isNextCharEmptySpace(tcpfd, qiext))
        {
            printf("QGR ERR\n");
            return;
        }
        readUntilChar(tcpfd, qiext + 1, ' ');
        if (isNextCharEmptySpace(tcpfd, qisize))
        {
            printf("QGR ERR\n");
            return;
        }
        readUntilChar(tcpfd, qisize + 1, ' ');
        sprintf(filename, "client/%s/%s.%s", active_topic.name, question, qiext);

        fp = fopen(filename, "wb");
        if (fp == NULL)
        {
            printf("Error creating question image file!\n\n");
            return;
        }
        size = atol(qisize);
        if (isNextCharEmptySpace(tcpfd, data))
        {
            printf("QGR ERR\n");
            return;
        }

        ptrData = data + 1;
        size--;
        fwrite(data, sizeof(char), 1, fp);
        while (size > 0)
        {
            bytesRead = readMax1024(tcpfd, ptrData, &size);
            fwrite(ptrData, sizeof(char), bytesRead, fp);
        }
        fclose(fp);
        readUntilChar(tcpfd, garbage, ' ');
    }
    if (isNextCharEmptySpace(tcpfd, N))
    {
        printf("QGR ERR\n");
        return;
    }
    if (N[0] == '0')
    {
        readUntilChar(tcpfd, garbage, '\n');
    }
    else
    {
        if (N[0] == '1')
        {
            readUntilChar(tcpfd, N + 1, ' ');
        }
        else
        {
            readUntilChar(tcpfd, garbage, ' ');
        }
        numOfAnswers = atoi(N);
        for (i = 0; i < numOfAnswers; i++)
        {
            if (isNextCharEmptySpace(tcpfd, aNumber))
            {
                printf("QGR ERR\n");
                return;
            }

            readUntilChar(tcpfd, aNumber + 1, ' ');

            if (isNextCharEmptySpace(tcpfd, aUserID))
            {
                printf("QGR ERR\n");
                return;
            }
            readUntilChar(tcpfd, aUserID + 1, ' ');
            if (isNextCharEmptySpace(tcpfd, asize))
            {
                printf("QGR ERR\n");
                return;
            }
            readUntilChar(tcpfd, asize + 1, ' ');
            sprintf(filename, "client/%s/%s_%s.txt", active_topic.name, question, aNumber);
            fp = fopen(filename, "w");
            if (fp == NULL)
            {
                printf("Error creating answer file!\n\n");
                return;
            }
            size = atol(asize);
            if (isNextCharEmptySpace(tcpfd, data))
            {
                printf("QGR ERR\n");
                return;
            }
            ptrData = data + 1;
            size--;
            fwrite(data, sizeof(char), 1, fp);
            while (size > 0)
            {
                bytesRead = readMax1024(tcpfd, ptrData, &size);
                fwrite(ptrData, sizeof(char), bytesRead, fp);
            }
            fclose(fp);

            readUntilChar(tcpfd, garbage, ' '); // discard space after \n
            if (isNextCharEmptySpace(tcpfd, aIMG))
            {
                printf("QGR ERR\n");
                return;
            }
            if ((i == numOfAnswers - 1) && aIMG[0] == '0')
            {
                break;
            }
            if (atoi(aIMG))
            {
                readUntilChar(tcpfd, garbage, ' ');
                if (isNextCharEmptySpace(tcpfd, aiext))
                {
                    printf("QGR ERR\n");
                    return;
                }
                readUntilChar(tcpfd, aiext + 1, ' ');
                aiext[3] = '\0';
                if (isNextCharEmptySpace(tcpfd, aisize))
                {
                    printf("QGR ERR\n");
                    return;
                }
                readUntilChar(tcpfd, aisize + 1, ' ');
                sprintf(filename, "client/%s/%s_%s.%s", active_topic.name, question, aNumber, aiext);
                fp = fopen(filename, "w");
                if (fp == NULL)
                {
                    printf("Error creating answer image file!\n\n");
                    return;
                }
                size = atol(aisize);
                if (isNextCharEmptySpace(tcpfd, data))
                {
                    printf("QGR ERR\n");
                    return;
                }
                ptrData = data + 1;
                size--;
                fwrite(data, sizeof(char), 1, fp);
                while (size > 0)
                {
                    bytesRead = readMax1024(tcpfd, ptrData, &size);
                    fwrite(ptrData, sizeof(char), bytesRead, fp);
                }
                fclose(fp);
                if (i != numOfAnswers - 1)
                {
                    if (!isNextCharEmptySpace(tcpfd, data))
                    {
                        printf("QGR ERR\n");
                        return;
                    }
                }
            }
            else
            {
                if (!isNextCharEmptySpace(tcpfd, data))
                {
                    printf("QGR ERR\n");
                    return;
                }
            }
        }
        readUntilChar(tcpfd, garbage, '\n');
    }
    printf("Question: %s successfully downloaded\n\n", question);
    strcpy(active_question, question);
    close(tcpfd);
    free(data);
}
void questionSubmit(int tcpfd, char *inputptr, char *response, struct addrinfo *restcp, int userid, Topic active_topic, char *active_question)
{
    FILE *fp;
    int fpfd;
    ssize_t size = 0, bytesRead = 0;
    long int message_length;
    char question[16], filename[NAME_MAX + 3], imagefilename[NAME_MAX + 3],
        *data, iext[8], *message, *ptrData;

    data = (char*)malloc(sizeof(char) * 1025);
    imagefilename[0] = '\0';
    sscanf(inputptr, "%s %s %s", question, filename, imagefilename);
    strcpy(active_question, question);

    openAndConnectToSocketTCP(restcp, &tcpfd);

    strcat(filename, ".txt");
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Question File Not Found!\n\n");
        return;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    message_length = 3 + 1 + 5 + 1 + strlen(active_topic.name) +
                     1 + strlen(question) + 1 + getNumberOfDigits(size) + 1;
    message = (char*)malloc(sizeof(char) * (message_length + 1));
    sprintf(message, "QUS %d %s %s %ld ",
            userid, active_topic.name, question, size);
    writeMax1024(tcpfd, message, &message_length);
    free(message);
    ptrData = data;
    fpfd = fileno(fp);
    while (size > 0)
    {
        bytesRead = readMax1024(fpfd, ptrData, &size);
        writeMax1024(tcpfd, ptrData, &bytesRead);
    }
    fclose(fp);
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
        size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        message_length = 1 + 1 + 1 + 3 + 1 + getNumberOfDigits(size) + 1;
        message = (char*) malloc((message_length+1) * sizeof(char));
        sprintf(message, " 1 %s %ld ", iext, size);
        writeMax1024(tcpfd, message, &message_length);
        while (size > 0)
        {
            bytesRead = readMax1024(fileno(fp), data, &size);
            writeMax1024(tcpfd, data, &bytesRead);
        }
        size = 1;
        writeMax1024(tcpfd, "\n", &size);
        fclose(fp);
        free(message);
    }
    else
    {
        size = 3;
        writeMax1024(tcpfd, " 0\n", &size);
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
    free(data);
}

void answerSubmit(int tcpfd, char *inputptr, char *response, struct addrinfo *restcp,
                  int userid, Topic active_topic, char *active_question)
{
    FILE *fp;
    long int message_length;
    ssize_t bytesRead;
    int fpfd;
    char filename[NAME_MAX + 3], imagefilename[NAME_MAX + 3], iext[4], *message;
    char *data, *ptrData;
    ssize_t asize = 0, isize = 0;

    data = (char*) malloc(sizeof(char)*1025);

    imagefilename[0] = '\0';
    sscanf(inputptr, "%s %s", filename, imagefilename);

    openAndConnectToSocketTCP(restcp, &tcpfd);

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


    message_length = 3 + 1 + 5 + 1 + strlen(active_topic.name) +
                     1 + strlen(active_question) + 1 + getNumberOfDigits(asize) + 1;

    message = (char*)malloc(sizeof(char) * (message_length + 1));

    sprintf(message, "ANS %d %s %s %ld ", userid, active_topic.name, active_question, asize);
    writeMax1024(tcpfd, message, &message_length);
    free(message);
    ptrData = data;
    fpfd = fileno(fp);
    while (asize > 0)
    {
        bytesRead = readMax1024(fpfd, ptrData, &asize);
        writeMax1024(tcpfd, ptrData, &bytesRead);
    }
    fclose(fp);

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

        message_length = 1 + 1 + 1 + 3 + 1 + getNumberOfDigits(isize) + 1;
        message = (char*)malloc(sizeof(char) * (message_length + 1));
        sprintf(message, " 1 %s %ld ", iext, isize);

        writeMax1024(tcpfd, message, &message_length); //write of everything, except image content
        while (isize > 0)
        {
            bytesRead = readMax1024(fileno(fp), data, &isize);
            writeMax1024(tcpfd, data, &bytesRead);
        }
        isize = 1;
        writeMax1024(tcpfd, "\n", &isize);
        fclose(fp);
        free(message);
    }
    else
    {
        isize = 3;
        writeMax1024(tcpfd, " 0\n", &isize);
    }

    readUntilChar(tcpfd, response, '\n');

    if (!strcmp(response, "ANR NOK\n"))
        printf("Failed to submit answer\n\n");
    else if (!strcmp(response, "ANR FUL\n"))
        printf("Failed to submit question - answer list full\n\n");
    else
        printf("Answer submited successfully\n\n");
    close(tcpfd);
    free(data);
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
    Topic topic_list[99 + 1]; // active_topic; // First entry is left empty to facilitate indexing
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;

    int tcpfd = 0, udpfd = 0, active_topic_number = 0, number_of_topics = 0, userid = 0;
    struct sigaction act;
    struct timeval tv;

    char port[16], hostname[128], buffer[128], input[128], command[128], response[2048], active_question[16];
    char *inputptr, *message;

    tv.tv_sec = 5;
    tv.tv_usec = 100000;

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

    if (setsockopt(udpfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        exit(EXIT_FAILURE);
    while (1)
    {
        printf("Enter command: ");
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
        {
            if (userid == 0)
                printf("Please register your userID first\n");
            else
                questionSubmit(tcpfd, inputptr, response, restcp, userid, topic_list[active_topic_number], active_question);
        }

        else if (!strcmp(command, "answer_submit") || !strcmp(command, "as"))
        {
            if (userid == 0)
                printf("Please register your userID first\n");
            else
                answerSubmit(tcpfd, inputptr, response, restcp, userid, topic_list[active_topic_number], active_question);
        }

        else
            printf("Unknown command\n\n");

        memset(message, 0, sizeof(char) * 2048);
        memset(response, 0, sizeof(char) * 2048);
        memset(buffer, 0, sizeof buffer);
        memset(input, 0, sizeof input);
        memset(command, 0, sizeof command);
    }
}