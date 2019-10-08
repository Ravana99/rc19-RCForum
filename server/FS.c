/* Group 39 - Joao Fonseca 89476, Tiago Pires 89544, Tomas Lopes 89552 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>

#define max(A, B) ((A) >= (B) ? (A) : (B))

int topic_amount = 0;

int isAlphanumeric(char *str)
{
    int i;
    for (i = 0; str[i] != '\0'; i++)
        if (!isalpha(str[i]) && !isdigit(str[i]))
            return 0;
    return 1;
}

int receiveudp(int udpfd, char *buffer, struct sockaddr_in *cliaddr, socklen_t *len)
{
    char request[128], response[2048];
    int id, n;

    if ((n = recvfrom(udpfd, buffer, 2048, 0, (struct sockaddr *)cliaddr, len)) == -1)
        return -1;
    buffer[n] = '\0'; // Appends a '\0' to the message so it can be used in strcmp
    sscanf(buffer, "%s", request);

    if (!strcmp(request, "REG"))
    {
        sscanf(buffer, "REG %d", &id);
        printf("User %d (IP %s) trying to register... ", id, inet_ntoa(cliaddr->sin_addr));
        if (id > 9999 && id < 100000)
        {
            strcpy(response, "RGR OK\n");
            printf("success\n");
        }
        else
        {
            strcpy(response, "RGR NOK\n");
            printf("denied\n");
        }
    }
    else if (!strcmp(request, "LTP"))
    {

        DIR *dir;
        struct dirent *entry;
        char topic_buffer[128], topic_name[16], topic_id[16];
        char delim[2] = "_";

        printf("User is listing the available topics\n");
        sprintf(response, "LTR %d", topic_amount);

        if ((dir = opendir("server/topics")) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            strtok(entry->d_name, delim);
            strcpy(topic_name, strtok(NULL, delim));
            strcpy(topic_id, strtok(NULL, delim));
            sprintf(topic_buffer, " %s:%s", topic_name, topic_id);
            strcat(response, topic_buffer);
        }
        strcat(response, "\n");

        closedir(dir);
    }
    else if (!strcmp(request, "PTP"))
    {
        DIR *dir;
        struct dirent *entry;
        char topic_name[128], existing_topic[16], pathname[128];
        int duplicate_topic = 0;
        char delim[2] = "_";
        sscanf(buffer, "PTP %d %s", &id, topic_name);

        printf("User %d is proposing topic %s... ", id, topic_name);

        if (strlen(topic_name) > 10 || !isAlphanumeric(topic_name))
        {
            printf("invalid\n");
            strcpy(response, "PTR NOK\n");
        }
        else
        {

            if ((dir = opendir("server/topics")) == NULL)
                return -1;
            readdir(dir); // Skips directory .
            readdir(dir); // Skips directory ..
            while ((entry = readdir(dir)) != NULL)
            {
                strtok(entry->d_name, delim);
                strcpy(existing_topic, strtok(NULL, delim));
                if (!strcmp(existing_topic, topic_name))
                {
                    duplicate_topic = 1;
                    break;
                }
            }
            if (topic_amount >= 99)
            {
                printf("full\n");
                strcpy(response, "PTR FUL\n");
            }
            else if (duplicate_topic)
            {
                printf("duplicate\n");
                strcpy(response, "PTR DUP\n");
            }
            else
            {
                printf("success\n");
                topic_amount++;
                if (topic_amount < 10)
                    sprintf(pathname, "server/topics/0%d_%s_%d", topic_amount, topic_name, id);
                else
                    sprintf(pathname, "server/topics/%d_%s_%d", topic_amount, topic_name, id);
                printf("%s\n", pathname);
                if (mkdir(pathname, 0666) == -1) // Enables R/W for all users
                    return -1;
                strcpy(response, "PTR OK\n");
            }
            closedir(dir);
        }
    }
    else if (!strcmp(request, "LQU"))
    {
        FILE *file;
        DIR *dir;
        struct dirent *entry;
        char topic_buffer[128], topic_cmp[16], topic_folder[32], topic_name[16], pathname[512], answer_path[512], question_name[16], question_id[16];
        char delim[2] = "_";
        int question_amount = 0, answer_amount = 0;

        sscanf(buffer, "LQU %s", topic_name);

        printf("User is listing the available questions for the topic %s\n", topic_name);
        if ((dir = opendir("server/topics")) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            strcpy(topic_folder, entry->d_name);
            strtok(entry->d_name, delim);
            strcpy(topic_cmp, strtok(NULL, delim));
            if (!strcmp(topic_cmp, topic_name))
            {
                sprintf(pathname, "server/topics/%s", topic_folder);
                break;
            }
        }
        closedir(dir);

        if ((dir = opendir(pathname)) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
            question_amount++;
        closedir(dir);
        sprintf(response, "LQR %d", question_amount);

        if ((dir = opendir(pathname)) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            sprintf(answer_path, "%s/%s/anscount.txt", pathname, entry->d_name);
            if ((file = fopen(answer_path, "r")) == NULL)
                exit(1);
            fscanf(file, "%d\n", &answer_amount);

            strtok(entry->d_name, delim);
            strcpy(question_name, strtok(NULL, delim));
            strcpy(question_id, strtok(NULL, delim));
            sprintf(topic_buffer, " %s:%s:%d", question_name, question_id, answer_amount);
            strcat(response, topic_buffer);
        }
        strcat(response, "\n");

        closedir(dir);
    }
    else
    {
        strcpy(response, "ERR\n");
    }

    return sendto(udpfd, response, strlen(response), 0, (struct sockaddr *)cliaddr, *len);
}

// NEEDS TESTING
void write_full(int fd, char *message)
{
    int nw;
    int n = strlen(message);
    char *messageptr = &message[0];
    while (n > 0)
    {
        if ((nw = write(fd, messageptr, n)) <= 0)
            exit(1);
        n -= nw;
        messageptr += nw;
    }
}

int receivetcp(int connfd, char *buffer, struct sockaddr_in *cliaddr, socklen_t *len)
{
    char request[128], response[2048];
    long buffer_size = 2048;
    int n;
    long realloc_aux;
    char *bufferptr;

    bufferptr = &buffer[0];

    do
    {
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            exit(1);
        bufferptr += n;
    } while (*(bufferptr - n) != ' ');

    sscanf(buffer, "%s ", request);

    if (!strcmp(request, "GQU"))
    {
        return 0;
    }
    else if (!strcmp(request, "QUS"))
    {

        int space_count = 0, question_count = 0, duplicate_question = 0;
        int quserid, n, fd, nw;
        long qsize;
        DIR *dir;
        struct dirent *entry;
        char delim[2] = "_";
        char *ptr;
        char topic[16], cmp[16], question[16], pathname[128], path_aux[128], topic_folder[32];

        do
        { // Reads until 4th space (between qsize and qdata)
            if ((n = read(connfd, bufferptr, 1)) <= 0)
                exit(1);
            if (*bufferptr == ' ')
                space_count++;
            bufferptr += n;
        } while (space_count < 4);

        // Scans command information
        sscanf(buffer, "QUS %d %s %s %ld", &quserid, topic, question, &qsize);

        realloc_aux = bufferptr - buffer;
        if ((buffer = (char *)realloc(buffer, (buffer_size + qsize) * sizeof(char))) == NULL)
            return -1;
        bufferptr = buffer + realloc_aux;

        printf("User %d is trying to submit question %s in topic %s... ", quserid, question, topic);

        if (strlen(topic) > 10 || !isAlphanumeric(topic))
        {
            printf("invalid\n");
            write_full(connfd, "QUR NOK\n");
            return 0;
        }

        // Opens topic folder
        if ((dir = opendir("server/topics")) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            strcpy(topic_folder, entry->d_name);
            strtok(entry->d_name, delim);
            strcpy(cmp, strtok(NULL, delim));
            if (!strcmp(cmp, topic))
            {
                sprintf(pathname, "server/topics/%s", topic_folder);
                break;
            }
        }
        closedir(dir);

        // Counts number of questions in topic and checks for duplicate
        if ((dir = opendir(pathname)) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            question_count++;
            strtok(entry->d_name, delim);
            strcpy(cmp, strtok(NULL, delim));
            if (!strcmp(cmp, question))
            {
                duplicate_question = 1;
                break;
            }
        }
        if (question_count >= 99)
        {
            printf("full\n");
            write_full(connfd, "QUR FUL\n");
            return 0;
        }
        else if (duplicate_question)
        {
            printf("duplicate\n");
            write_full(connfd, "QUR DUP\n");
            return 0;
        }
        closedir(dir);

        // Creates question folder
        if (question_count < 9)
            sprintf(pathname, "server/topics/%s/0%d_%s_%d", topic_folder, question_count + 1, question, quserid);
        else
            sprintf(pathname, "server/topics/%s/%d_%s_%d", topic_folder, question_count + 1, question, quserid);

        if (mkdir(pathname, 0666) == -1) // Enables R/W for all users
            return -1;

        // Creates answer count file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/anscount.txt");
        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;
        if (write(fd, "0", 1) != 1)
            return -1;
        if (write(fd, "\n", 1) != 1)
            return -1;
        if (close(fd) < 0)
            return -1;

        // Creates question info file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/qinfo.txt");
        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;
        while ((n = read(connfd, bufferptr, qsize)) != 0)
        {
            if (n == -1)
                return -1;
            ptr = bufferptr;
            qsize -= n;

            while (n > 0)
            {
                if ((nw = write(fd, ptr, n)) <= 0)
                    return -1;
                n -= nw;
                ptr += nw;
            }
            bufferptr += n;
        }

        if (close(fd) < 0)
            return -1;

        // Reads space between qdata and qIMG
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        bufferptr += n;

        // Reads qIMG
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        bufferptr += n;

        if (*(bufferptr - n) == '1') // Has image to read
        {
            char *ptr_aux = bufferptr;
            char iext[4];
            long isize;
            space_count = 0;
            do // Reads 3 more spaces (until the space between isize and idata)
            {
                if ((n = read(connfd, bufferptr, 1)) <= 0)
                    exit(1);
                if (*bufferptr == ' ')
                    space_count++;
                bufferptr += n;
            } while (space_count < 3);

            sscanf(ptr_aux, " %s %ld ", iext, &isize);

            realloc_aux = bufferptr - buffer;
            if ((buffer = (char *)realloc(buffer, (buffer_size + qsize + isize) * sizeof(char))) == NULL)
                return -1;
            bufferptr = buffer + realloc_aux;

            strcpy(path_aux, pathname);
            strcat(path_aux, "/qimg.");
            strcat(path_aux, iext);

            if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
                return -1;
            while ((n = read(connfd, bufferptr, isize)) != 0)
            {
                if (n == -1)
                    return -1;
                ptr = bufferptr;
                isize -= n;
                while (n > 0)
                {
                    if ((nw = write(fd, ptr, n)) <= 0)
                        return -1;
                    n -= nw;
                    ptr += nw;
                }
                bufferptr += n;
            }
            if (close(fd) < 0)
                return -1;

            if ((n = read(connfd, bufferptr, 1)) <= 0)
                return -1;
            if (*bufferptr == '\n')
            {
                printf("success\n");
                write_full(connfd, "QUR OK\n");
                return 0;
            }
            else
            {
                printf("failure\n");
                write_full(connfd, "QUR NOK\n");
                return 0;
            }
        }
        else if (*(bufferptr - n) == '0') // Has no image to read
        {
            if ((n = read(connfd, bufferptr, 1)) <= 0)
                return -1;
            if (*bufferptr == '\n')
            {
                printf("success\n");
                write_full(connfd, "QUR OK\n");
                return 0;
            }
            else
            {
                printf("failure\n");
                write_full(connfd, "QUR NOK\n");
                return 0;
            }
        }
        else
        {
            printf("failure\n");
            write_full(connfd, "QUR NOK\n");
            return 0;
        }
    }
    else if (!strcmp(request, "ANS"))
    {
        int space_count = 0, answer_count = 0;
        int auserid, n, fd, nw;
        long asize;
        DIR *dir;
        struct dirent *entry;
        char delim[2] = "_";
        char *ptr;
        char topic[16], cmp[16], question[16], pathname[128], path_aux[128], topic_folder[32], question_folder[32], ansf[4], answer_name[32];

        do
        { // Reads until 4th space (between asize and adata)
            if ((n = read(connfd, bufferptr, 1)) <= 0)
                exit(1);
            if (*bufferptr == ' ')
                space_count++;
            bufferptr += n;
        } while (space_count < 4);

        // Scans command information
        sscanf(buffer, "ANS %d %s %s %ld", &auserid, topic, question, &asize);

        realloc_aux = bufferptr - buffer;
        if ((buffer = (char *)realloc(buffer, (buffer_size + asize) * sizeof(char))) == NULL)
            return -1;
        bufferptr = buffer + realloc_aux;

        printf("User %d is trying to submit answer for question %s in topic %s... ", auserid, question, topic);

        // Opens topic folder
        if ((dir = opendir("server/topics")) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            strcpy(topic_folder, entry->d_name);
            strtok(entry->d_name, delim);
            strcpy(cmp, strtok(NULL, delim));
            if (!strcmp(cmp, topic))
            {
                sprintf(pathname, "server/topics/%s", topic_folder);
                break;
            }
        }
        closedir(dir);

        // Opens question folder
        if ((dir = opendir(pathname)) == NULL)
            return -1;
        readdir(dir); // Skips directory .
        readdir(dir); // Skips directory ..
        while ((entry = readdir(dir)) != NULL)
        {
            strcpy(question_folder, entry->d_name);
            strtok(entry->d_name, delim);
            strcpy(cmp, strtok(NULL, delim));
            if (!strcmp(cmp, question))
            {
                strcat(pathname, "/");
                strcat(pathname, question_folder);
                break;
            }
        }
        closedir(dir);

        // Checking number of answers
        strcpy(path_aux, pathname);
        strcat(path_aux, "/anscount.txt");

        if ((fd = open(path_aux, O_RDONLY)) < 0)
            return -1;
        if (read(fd, ansf, 2) != 2)
            return -1;
        if (close(fd) < 0)
            return -1;

        answer_count = atoi(ansf);

        if (answer_count >= 99)
        {
            write_full(connfd, "ANR FUL\n");
            return 0;
        }
        else
        {
            if ((fd = open(path_aux, O_WRONLY | O_TRUNC)) < 0)
                return -1;
            answer_count++;
            sprintf(ansf, "%d\n", answer_count);
            if (write(fd, ansf, (answer_count > 9 ? 3 : 2)) != (answer_count > 9 ? 3 : 2))
                return -1;

            if (close(fd) < 0)
                return -1;
        }

        // Creates answer text file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        if (answer_count > 9)
            sprintf(answer_name, "%d_%d.txt", answer_count, auserid);
        else
            sprintf(answer_name, "0%d_%d.txt", answer_count, auserid);
        strcat(path_aux, answer_name);

        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;
        while ((n = read(connfd, bufferptr, asize)) != 0)
        {
            if (n == -1)
                return -1;
            ptr = bufferptr;
            asize -= n;

            while (n > 0)
            {
                if ((nw = write(fd, ptr, n)) <= 0)
                    return -1;
                n -= nw;
                ptr += nw;
            }
            bufferptr += n;
        }

        if (close(fd) < 0)
            return -1;

        // Reads space between adata and aIMG
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        bufferptr += n;

        // Reads aIMG
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        bufferptr += n;

        if (*(bufferptr - n) == '1') // Has image to read
        {
            char *ptr_aux = bufferptr;
            char iext[4];
            long isize;
            space_count = 0;
            do // Reads 3 more spaces (until the space between isize and idata)
            {
                if ((n = read(connfd, bufferptr, 1)) <= 0)
                    exit(1);
                if (*bufferptr == ' ')
                    space_count++;
                bufferptr += n;
            } while (space_count < 3);

            sscanf(ptr_aux, " %s %ld ", iext, &isize);

            realloc_aux = bufferptr - buffer;
            if ((buffer = (char *)realloc(buffer, (buffer_size + asize + isize) * sizeof(char))) == NULL)
                return -1;
            bufferptr = buffer + realloc_aux;

            strcpy(path_aux, pathname);
            strcat(path_aux, "/");
            if (answer_count > 9)
                sprintf(answer_name, "%d_%d.%s", answer_count, auserid, iext);
            else
                sprintf(answer_name, "0%d_%d.%s", answer_count, auserid, iext);
            strcat(path_aux, answer_name);
            

            if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
                return -1;
            while ((n = read(connfd, bufferptr, isize)) != 0)
            {
                if (n == -1)
                    return -1;
                ptr = bufferptr;
                isize -= n;
                while (n > 0)
                {
                    if ((nw = write(fd, ptr, n)) <= 0)
                        return -1;
                    n -= nw;
                    ptr += nw;
                }
                bufferptr += n;
            }
            if (close(fd) < 0)
                return -1;

            if ((n = read(connfd, bufferptr, 1)) <= 0)
                return -1;
            if (*bufferptr == '\n')
            {
                printf("success\n");
                write_full(connfd, "ANR OK\n");
                return 0;
            }
            else
            {
                printf("failure\n");
                write_full(connfd, "ANR NOK\n");
                return 0;
            }
        }
        else if (*(bufferptr - n) == '0') // Has no image to read
        {
            if ((n = read(connfd, bufferptr, 1)) <= 0)
                return -1;
            if (*bufferptr == '\n')
            {
                printf("success\n");
                write_full(connfd, "ANR OK\n");
                return 0;
            }
            else
            {
                printf("failure\n");
                write_full(connfd, "ANR NOK\n");
                return 0;
            }
        }
        else
        {
            printf("failure\n");
            write_full(connfd, "ANR NOK\n");
            return 0;
        }
    }
    else
    {
        printf("Unexpected protocol message received\n");
        write_full(connfd, "ERR\n");
        return 0;
    }
}

int main(int argc, char **argv)
{

    int listenfd, connfd, udpfd, nready, maxfdp1, errcode;
    char buffer[2048], port[16];
    fd_set rset;
    struct sockaddr_in cliaddr;
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    socklen_t len;
    struct sigaction act, act2;
    pid_t childpid;
    DIR *dir;
    struct dirent *entry;

    memset(buffer, 0, sizeof buffer);

    memset(&hintstcp, 0, sizeof hintstcp);
    hintstcp.ai_family = AF_INET;
    hintstcp.ai_socktype = SOCK_STREAM;
    hintstcp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    memset(&hintsudp, 0, sizeof hintsudp);
    hintsudp.ai_family = AF_INET;
    hintsudp.ai_socktype = SOCK_DGRAM;
    hintsudp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
        exit(1);
    memset(&act2, 0, sizeof act);
    act2.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act2, NULL) == -1)
        exit(1);

    strcpy(port, "58039");
    if (argc == 3)
    {
        if (!strcmp(argv[1], "-p"))
            strcpy(port, argv[2]);
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

    if ((dir = opendir("server/topics")) == NULL)
        exit(1);
    while ((entry = readdir(dir)) != NULL)
        topic_amount++;
    topic_amount -= 2; // Disregards directories . and ..
    closedir(dir);

    if ((errcode = getaddrinfo(NULL, port, &hintstcp, &restcp)) != 0)
        exit(1);
    if ((errcode = getaddrinfo(NULL, port, &hintsudp, &resudp)) != 0)
        exit(1);

    /* tcp socket */
    if ((listenfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
        exit(1);
    //if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit(1);
    if (bind(listenfd, restcp->ai_addr, restcp->ai_addrlen) == -1)
        exit(1);
    if (listen(listenfd, 5) == -1)
        exit(1);

    /* udp socket */
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1)
        exit(1);
    if ((errcode = bind(udpfd, resudp->ai_addr, resudp->ai_addrlen)) == -1)
        exit(1);
    //if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) exit(1);

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;

    while (1)
    {

        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        if ((nready = select(maxfdp1 + 1, &rset, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL)) <= 0)
            exit(1);

        if (FD_ISSET(listenfd, &rset))
        {
            len = sizeof(cliaddr);
            do
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
            while (connfd == -1 && errno == EINTR);
            if (connfd == -1)
                exit(1);

            if ((childpid = fork()) == 0)
            {
                char *tcpbuffer;
                if ((tcpbuffer = (char *)malloc(2048 * sizeof(char))) == NULL)
                    exit(1);
                close(listenfd);
                memset(tcpbuffer, 0, 2048);

                if (receivetcp(connfd, tcpbuffer, &cliaddr, &len) == -1)
                    exit(1);
                else
                    exit(0);
            }
            else if (childpid == -1)
                exit(1);
            do
                errcode = close(connfd);
            while (errcode == -1 && errno == EINTR);
            if (errcode == -1)
                exit(1);
        }

        if (FD_ISSET(udpfd, &rset))
        {
            len = sizeof(cliaddr);
            memset(buffer, 0, sizeof buffer);
            if (receiveudp(udpfd, buffer, &cliaddr, &len) == -1)
                exit(1);
        }
    }

    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(listenfd);
    close(udpfd);

    exit(0);
}
