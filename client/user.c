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
    for (i = 0; i < imagefilename_length; i++)
    {
        if (imagefilename[i] == '.')
            break;
    }
    for (i++; i < imagefilename_length; i++)
    {
        if (j == 3)
            break;
        iext[j++] = imagefilename[i];
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
        exit(EXIT_FAILURE);
    response[n] = '\0'; // Appends a '\0' to the response so it can be used in strcmp
    return n;
}

void sendTCP(struct addrinfo *restcp, char *message, char *response, int message_length)
{
    ssize_t nbytes = message_length * sizeof(char), nleft, nwritten, nread;
    char *ptr, answer[2048];
    int tcpfd;

    if ((tcpfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
    {
        printf("Socket error\n");
        exit(EXIT_FAILURE);
    }
    if (connect(tcpfd, restcp->ai_addr, restcp->ai_addrlen) == -1)
    {
        printf("Connection error\n");
        exit(1);
    }

    nleft = nbytes;
    //printf("nbytes %zu\n%s\nresponse %s\n", nbytes, message, response);
    ptr = message;
    while (nleft > 0)
    {
        write(1, "1\n", 2);
        if ((nwritten = write(tcpfd, ptr, nleft)) <= 0)
        {
            write(1, "WERROR\n", 7);
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    ptr = response;
    // nleft = nbytes;
    do
    {
        if ((nread = read(tcpfd, ptr, 1)) <= 0)
        {
            write(1, "RERROR\n", 7);
            exit(EXIT_FAILURE);
        }
        ptr += nread;
    } while (*(ptr - nread) != '\n');

    //nread = nbytes - nleft;
    printf("nbytes %zu\n%s\n\n", nbytes, message);
    close(tcpfd);
    //return response
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
void questionSubmit(char *inputptr, char *message, char *response, struct addrinfo *restcp, int userid, int active_topic_number, Topic *topic_list)
{
    FILE *fp;
    int i, message_length;
    char question[11], filename[NAME_MAX + 3], imagefilename[NAME_MAX + 3], *ptr, messageTMP[2048], qdata[2048], *idata, iext[4];
    imagefilename[0] = '\0';
    sscanf(inputptr, "%s %s %s", question, filename, imagefilename);
    ssize_t qsize = 0, isize = 0, nbytes = 0, nleft = 0, nwritten = 0, nread = 0;

    sscanf(inputptr, "%s %s %s", question, filename, imagefilename);
    //printf("INPUT:%s\nNAMELIMIT:%d\nfilename:%s\nquestion:l%s\nimagefilename:%s\n", inputptr, NAME_MAX, filename, question, imagefilename);
    strcat(filename, ".txt");
    fp = fopen(filename, "r");
    message_length = sizeof(userid) + strlen(topic_list[active_topic_number].name) + 1 + strlen(question) + 1 + sizeof(qsize) + strlen(qdata) + 1;
    if (fp == NULL)
    {
        printf("Question File Not Found!\n\n");
        return;
    }
    fseek(fp, 0L, SEEK_END);
    qsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fread(qdata, sizeof(char) * qsize, 1, fp);
    strtok(qdata, "\n");
    fclose(fp);
    if (imagefilename[0] != '\0')
    {
        //printf("HERE File Not Found!\n\n");
        fp = fopen(imagefilename, "rb");
        if (fp == NULL)
        {
            printf("Image File Not Found!\n\n");
            return;
        }
        getImageExtension(imagefilename, iext);

        //printf("%s\n", idata);
        fseek(fp, 0L, SEEK_END);
        isize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        idata = malloc(sizeof(char) * (isize + 1));
        message = realloc(message, sizeof(char) * isize + sizeof(char) * message_length + 15);
        message_length += 15 + isize;
        fread(idata, sizeof(char) * isize, 1, fp);
        fclose(fp);

        /*TESTAR SE IMAGEM ESTA BEM
        fp = fopen("damn.jpg", "wb");
        fwrite(idata, sizeof(char) * isize, 1, fp);
        fclose(fp);
        */
        /*for (int i = 1; i < 61; i++)
        {
            printf("%s\n", topic_list[i].name);
        }*/
        sprintf(message, "QUS %d %s %s %ld %s 1 %s %ld %s\n", userid, topic_list[active_topic_number].name, question, qsize, qdata, iext, isize, idata);
    }
    else
    {
        message = realloc(message, sizeof(char) * (message_length + 11));
        message_length += 11;
        sprintf(message, "QUS %d %s %s %ld %s 0\n", userid, topic_list[active_topic_number].name, question, qsize, qdata);
    }
    sendTCP(restcp, message, response, message_length);
    printf("RESPONSE %s\n\n", response);
    //free(idata);
}
void answerSubmit()
{
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
    ssize_t n, nbytes, nleft, nwritten, nread;

    int udpfd, errcode, active_topic_number = 0, number_of_topics = 0, userid = 0;
    struct sigaction act;

    char port[16], hostname[128], buffer[128], input[128], command[128], response[2048], active_question[11];
    char *inputptr, *ptr, *message;

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

        else if (!strcmp(command, "question_list"))
            questionList(message, response, udpfd, resudp,
                         active_topic_number, number_of_topics, topic_list);

        else if (!strcmp(command, "question_get"))
            questionGet(message, response, restcp, active_topic_number, topic_list, false);
        else if (!strcmp(command, "qg"))
           questionGet(message, response, restcp, active_topic_number, topic_list, true);

        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs"))
            questionSubmit(inputptr, message, response, restcp, userid, active_topic_number, topic_list);

        else if (!strcmp(command, "answer_submit") || !strcmp(command, "as"))
            answerSubmit();

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