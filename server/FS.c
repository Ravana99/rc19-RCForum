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
#define BUFF_MAX 2048
#define P_MAX 512
#define F_MAX 64
#define ID_MAX 8
#define EXT_MAX 8

int topic_amount = 0;

// ------ AUXILIARY FUNCTIONS ------ //

// Checks if string is made up of only alphanumeric characters
int isAlphanumeric(char *str)
{
    int i;
    for (i = 0; str[i] != '\0'; i++)
        if (!isalpha(str[i]) && !isdigit(str[i]))
            return 0;
    return 1;
}

int getNumberOfDigits(long n)
{
    int count = 0;
    while (n != 0)
    {
        n /= 10;
        count++;
    }
    return count;
}

void reallocate(long *size, long inc, char **str, char **ptr)
{
    long aux;
    *size += inc;
    aux = *ptr - *str;
    if ((*str = (char *)realloc(*str, *size * sizeof(char))) == NULL)
        exit(1);
    *ptr = *str + aux;
}

void appendString(char **ptr, char *str, int n)
{
    memcpy(*ptr, str, n);
    *ptr += n;
}

// Reads bytes from fd, 1 by 1, until it reaches the character 'token' for the nth time
void readUntil(int fd, char **bufferptr, int n, char token)
{
    int i, nr;
    for (i = 0; i < n; i++)
    {
        do
        { // Reads until '\n'
            if ((nr = read(fd, *bufferptr, 1)) <= 0)
                exit(1);
            *bufferptr += nr;
        } while (*(*bufferptr - nr) != token);
    }
}

// Reads n bytes from fd, in chunks as large as the system can
void readFull(int fd, char **ptr, long n)
{
    int nr;
    while (n > 0)
    {
        if ((nr = read(fd, *ptr, n)) <= 0)
            exit(1);
        n -= nr;
        *ptr += nr;
    }
}

// Writes n bytes from fd, in chunks as large as the system can
void writeFull(int fd, char *str, long n)
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

// Reads command line arguments and sets the port number accordingly
void setPort(int argc, char **argv, char *port)
{
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
    else
        strcpy(port, "58039");
}

// Checks if a topic exists and saves its path (if pathname isn't NULL)
int findTopic(char *topic, char *pathname)
{
    DIR *dir;
    struct dirent *entry;
    char delim[2] = "_", topic_name[F_MAX], topic_folder[F_MAX];

    if ((dir = opendir("server/topics")) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(topic_folder, entry->d_name);
        strtok(entry->d_name, delim);
        strcpy(topic_name, strtok(NULL, delim));
        if (!strcmp(topic, topic_name))
        {
            if (pathname != NULL)
                sprintf(pathname, "server/topics/%s", topic_folder);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Finds a question folder in a certain topic, saves its path and
// saves the ID of the submitter (if id isn't NULL)
int findQuestion(char *question, char *pathname, int *id)
{
    DIR *dir;
    struct dirent *entry;
    char delim[2] = "_", question_name[F_MAX], question_folder[F_MAX], quserid[ID_MAX];

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(question_folder, entry->d_name);
        strtok(entry->d_name, delim);
        strcpy(question_name, strtok(NULL, delim));
        strcpy(quserid, strtok(NULL, delim));
        if (!strcmp(question_name, question))
        {
            strcat(pathname, "/");
            strcat(pathname, question_folder);
            if (id != NULL)
                *id = atoi(quserid);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Finds the image associated with a certain question
// (if there is one) and saves its extension
int findQImg(char *pathname, char *ext)
{
    DIR *dir;
    struct dirent *entry;
    char img_name[F_MAX], delim[2] = ".";

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(img_name, entry->d_name);
        strtok(img_name, delim);
        if (!strcmp(img_name, "qimg"))
        {
            strcpy(ext, strtok(NULL, delim));
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Finds a certain answer text file and saves the id of the submitter
int findAnswer(char *pathname, char *answer, int *id, int i)
{
    DIR *dir;
    struct dirent *entry;
    char i_str[4], delim[3] = "_.", aux[4], aid[ID_MAX];

    if (i < 10)
        sprintf(i_str, "0%d", i);
    else
        sprintf(i_str, "%d", i);

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(answer, entry->d_name);
        strcpy(aux, strtok(entry->d_name, delim));
        strcpy(aid, strtok(NULL, delim)); // Gets user ID
        if (!strcmp(i_str, aux))
        {
            strtok(answer, ".");
            *id = atoi(aid);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Finds the image associated with a certain answer
// (if there is one) and saves its extension
int findAImg(char *pathname, char *answer, char *aiext)
{
    DIR *dir;
    struct dirent *entry;
    char delim[2] = ".", aux[16];

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(aux, entry->d_name);
        strtok(aux, delim);
        strcpy(aiext, strtok(NULL, delim));

        if (!strcmp(answer, aux) && strcmp(aiext, "txt"))
        {
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

// Returns number of questions in a given topic or -1 if it finds a duplicate
int getQuestionCount(char *pathname, char *question)
{
    DIR *dir;
    struct dirent *entry;
    int question_count = 0;
    char aux[32], delim[2] = "_";

    if ((dir = opendir(pathname)) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        question_count++;
        strtok(entry->d_name, delim);
        strcpy(aux, strtok(NULL, delim));
        if (!strcmp(aux, question))
        {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);

    return question_count;
}

// Returns number of answers in a question given a question path
int getAnswerCount(char *pathname)
{
    FILE *fp;
    int answer_count;
    char answer_path[P_MAX];

    strcpy(answer_path, pathname);
    strcat(answer_path, "/anscount.txt");

    if ((fp = fopen(answer_path, "r")) == NULL)
        exit(1);

    fscanf(fp, "%d\n", &answer_count);
    fclose(fp);

    return answer_count;
}

// Returns the file size, in bytes, of a given file
long getFileSize(char *pathname, char *filename)
{
    FILE *fp;
    long size;
    char path_aux[P_MAX];

    strcpy(path_aux, pathname);
    strcat(path_aux, "/");
    strcat(path_aux, filename);
    if ((fp = fopen(path_aux, "r")) == NULL)
        exit(1);
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fclose(fp);

    return size;
}

// ------ COMMANDS ------ //

int registerUser(char *buffer, int *id, char *response, struct sockaddr_in *cliaddr)
{
    sscanf(buffer, "REG %d", id);
    printf("User %d (IP %s) trying to register... ", *id, inet_ntoa(cliaddr->sin_addr));
    if (*id > 9999 && *id < 100000)
    {
        strcpy(response, "RGR OK\n");
        printf("success\n");
    }
    else
    {
        strcpy(response, "RGR NOK\n");
        printf("denied\n");
    }
    return 0;
}

int topicList(char *response)
{
    DIR *dir;
    struct dirent *entry;
    char topic_buffer[F_MAX], topic_name[F_MAX], topic_id[ID_MAX];
    char delim[2] = "_";

    printf("User is listing the available topics\n");
    sprintf(response, "LTR %d", topic_amount);

    // Lists every directory
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
    return 0;
}

int topicPropose(char *buffer, int *id, char *response)
{
    char topic_name[F_MAX], pathname[P_MAX];

    sscanf(buffer, "PTP %d %s", id, topic_name);

    printf("User %d is proposing topic %s... ", *id, topic_name);

    if (strlen(topic_name) > 10 || !isAlphanumeric(topic_name))
    {
        printf("invalid\n");
        strcpy(response, "PTR NOK\n");
    }
    else
    {
        if (findTopic(topic_name, NULL)) // Checks if topic already exists
        {
            printf("duplicate\n");
            strcpy(response, "PTR DUP\n");
        }
        else if (topic_amount >= 99) // Checks if the maximum number of topics has been reached
        {
            printf("full\n");
            strcpy(response, "PTR FUL\n");
        }
        else
        {
            printf("success\n");
            topic_amount++;
            if (topic_amount < 10)
                sprintf(pathname, "server/topics/0%d_%s_%d", topic_amount, topic_name, *id);
            else
                sprintf(pathname, "server/topics/%d_%s_%d", topic_amount, topic_name, *id);
            printf("%s\n", pathname);
            if (mkdir(pathname, 0666) == -1) // Enables R/W for all users
                return -1;
            strcpy(response, "PTR OK\n");
        }
    }
    return 0;
}

int questionList(char *buffer, char *response)
{
    DIR *dir;
    struct dirent *entry;
    char question_buffer[F_MAX], topic_name[F_MAX], pathname[P_MAX], question_name[F_MAX];
    char question_id[ID_MAX], question_path[P_MAX], delim[2] = "_";
    int question_amount = 0, answer_amount = 0;

    sscanf(buffer, "LQU %s", topic_name);

    printf("User is listing the available questions for the topic %s\n", topic_name);

    findTopic(topic_name, pathname);

    // Counts number of questions in a topic
    if ((dir = opendir(pathname)) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
        question_amount++;
    closedir(dir);
    sprintf(response, "LQR %d", question_amount);

    // Lists all the questions in a topic
    if ((dir = opendir(pathname)) == NULL)
        return -1;
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
    {
        strcpy(question_path, pathname);
        strcat(question_path, "/");
        strcat(question_path, entry->d_name);
        answer_amount = getAnswerCount(question_path);

        strtok(entry->d_name, delim);
        strcpy(question_name, strtok(NULL, delim));
        strcpy(question_id, strtok(NULL, delim));
        sprintf(question_buffer, " %s:%s:%d", question_name, question_id, answer_amount);
        strcat(response, question_buffer);
    }
    closedir(dir);
    strcat(response, "\n");
    return 0;
}

int questionGet(int connfd, char *buffer)
{
    char topic[F_MAX], question[F_MAX], pathname[P_MAX], path_aux[P_MAX];
    char img[F_MAX], iext[EXT_MAX], aux[64];
    char *response, *responseptr, *bufferptr;
    int answer_count, fd, i, j, quserid = -1;
    long ressize = BUFF_MAX, qsize, isize;

    if ((response = (char *)malloc(ressize * sizeof(char))) == NULL)
        return -1;

    responseptr = &response[0];
    bufferptr = &buffer[4]; // Skips command

    readUntil(connfd, &bufferptr, 1, '\n');
    *bufferptr = '\0';

    if (sscanf(buffer, "GQU %s %s\n", topic, question) != 2) // Message in the wrong format
    {
        writeFull(connfd, "QGR ERR\n", strlen("QGR ERR\n"));
        free(response);
        free(buffer);
        printf("User is trying to get question... failure\n");
        return 0;
    }

    printf("User is trying to get question %s of topic %s... ", question, topic);

    // Opens topic folder
    strcpy(pathname, "server/topics");
    if (!findTopic(topic, pathname))
    {
        writeFull(connfd, "QGR EOF\n", strlen("QGR EOF\n"));
        free(response);
        free(buffer);
        printf("not found\n");
        return 0;
    }

    // Opens question folder
    if (!findQuestion(question, pathname, &quserid))
    {
        writeFull(connfd, "QGR EOF\n", strlen("QGR EOF\n"));
        free(response);
        free(buffer);
        printf("not found\n");
        return 0;
    }

    answer_count = getAnswerCount(pathname);
    qsize = getFileSize(pathname, "qinfo.txt");

    reallocate(&ressize, qsize, &response, &responseptr);

    sprintf(response, "QGR %d %ld ", quserid, qsize);
    responseptr = response + strlen(response);

    // Opens question text file and reads it
    strcpy(path_aux, pathname);
    strcat(path_aux, "/qinfo.txt");
    if ((fd = open(path_aux, O_RDONLY)) < 0)
        return -1;
    readFull(fd, &responseptr, qsize);
    if (close(fd) < 0)
        return -1;

    appendString(&responseptr, " ", 1);

    // Opens image file if one exists
    if (findQImg(pathname, iext))
    {
        appendString(&responseptr, "1 ", 2);
        appendString(&responseptr, iext, 3);
        appendString(&responseptr, " ", 1);

        sprintf(img, "qimg.%s", iext);
        isize = getFileSize(pathname, img);

        reallocate(&ressize, isize, &response, &responseptr);

        sprintf(aux, "%ld", isize);
        appendString(&responseptr, aux, strlen(aux));
        appendString(&responseptr, " ", 1);

        // Opens image file and reads it
        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        strcat(path_aux, img);
        if ((fd = open(path_aux, O_RDONLY)) < 0)
            return -1;
        readFull(fd, &responseptr, isize);
        if (close(fd) < 0)
            return -1;

        appendString(&responseptr, " ", 1);
    }
    else
        appendString(&responseptr, "0 ", 2);

    sprintf(aux, "%d", answer_count);
    if (answer_count >= 10)
        appendString(&responseptr, "10", 2);
    else
        appendString(&responseptr, aux, 1);

    // Gets information of available answers
    for (i = answer_count, j = 0; i > 0 && j < 10; i--, j++)
    {
        char aiext[EXT_MAX], answer[F_MAX];
        long asize, aisize;
        int auserid;

        findAnswer(pathname, answer, &auserid, i);

        // Gets size of answer text file
        strcpy(aux, answer);
        strcat(aux, ".txt");
        asize = getFileSize(pathname, aux);

        reallocate(&ressize, asize, &response, &responseptr);

        if (i < 10)
            sprintf(aux, " 0%d %d %ld ", i, auserid, asize);
        else
            sprintf(aux, " %d %d %ld ", i, auserid, asize);
        appendString(&responseptr, aux, strlen(aux));

        // Opens answer text file and reads it
        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        strcat(path_aux, answer);
        strcat(path_aux, ".txt");
        if ((fd = open(path_aux, O_RDONLY)) < 0)
            return -1;
        readFull(fd, &responseptr, asize);
        if (close(fd) < 0)
            return -1;

        appendString(&responseptr, " ", 1);

        // Opens image file if one exists
        if (findAImg(pathname, answer, aiext))
        {
            char image_name[32];

            appendString(&responseptr, "1 ", 2);
            appendString(&responseptr, aiext, 3);
            appendString(&responseptr, " ", 1);

            strcpy(image_name, answer);
            strcat(image_name, ".");
            strcat(image_name, aiext);

            aisize = getFileSize(pathname, image_name);

            reallocate(&ressize, aisize, &response, &responseptr);

            sprintf(aux, "%ld", aisize);
            appendString(&responseptr, aux, strlen(aux));
            appendString(&responseptr, " ", 1);

            // Opens image file and reads it
            strcpy(path_aux, pathname);
            strcat(path_aux, "/");
            strcat(path_aux, image_name);
            if ((fd = open(path_aux, O_RDONLY)) < 0)
                return -1;
            readFull(fd, &responseptr, aisize);
            if (close(fd) < 0)
                return -1;
        }
        else
        {
            appendString(&responseptr, "0", 1);
        }
    }
    memcpy(responseptr, "\n", 1);
    writeFull(connfd, response, (long)(responseptr - response + 1));
    printf("success\n");
    free(response);
    free(buffer);
    return 0;
}

int questionSubmit(int connfd, char *buffer)
{
    int question_count, quserid, n, fd;
    long qsize, buffersize = BUFF_MAX;
    char *ptr, *bufferptr;
    char topic[F_MAX], question[F_MAX], pathname[P_MAX], path_aux[P_MAX], question_folder[F_MAX];

    bufferptr = &buffer[4]; // Skips command

    readUntil(connfd, &bufferptr, 4, ' ');

    // Scans command info
    *(bufferptr - 1) = '\0';
    sscanf(buffer, "QUS %d %s %s %ld", &quserid, topic, question, &qsize);
    *(bufferptr - 1) = ' ';

    if (getNumberOfDigits(qsize) > 10)
    {
        printf("invalid\n");
        writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
        free(buffer);
        return 0;
    }

    reallocate(&buffersize, qsize, &buffer, &bufferptr);

    printf("User %d is trying to submit question %s in topic %s... ", quserid, question, topic);

    if (strlen(question) > 10 || !isAlphanumeric(question))
    {
        printf("invalid\n");
        writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
        free(buffer);
        return 0;
    }

    findTopic(topic, pathname);

    question_count = getQuestionCount(pathname, question);

    if (question_count >= 99)
    {
        printf("full\n");
        writeFull(connfd, "QUR FUL\n", strlen("QUR FUL\n"));
        free(buffer);
        return 0;
    }
    else if (question_count == -1) // Found a duplicate
    {
        printf("duplicate\n");
        writeFull(connfd, "QUR DUP\n", strlen("QUR DUP\n"));
        free(buffer);
        return 0;
    }

    // Creates question folder
    if (question_count < 9)
        sprintf(question_folder, "0%d_%s_%d", question_count + 1, question, quserid);
    else
        sprintf(question_folder, "%d_%s_%d", question_count + 1, question, quserid);
    strcat(pathname, "/");
    strcat(pathname, question_folder);

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

    // Creates question info file and writes the data it's reading from the buffer to the file
    strcpy(path_aux, pathname);
    strcat(path_aux, "/qinfo.txt");
    if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
        return -1;
    ptr = bufferptr;
    readFull(connfd, &bufferptr, qsize);
    writeFull(fd, ptr, qsize);

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
        char iext[EXT_MAX];
        long isize;

        readUntil(connfd, &bufferptr, 3, ' ');

        // Scans file info
        *(bufferptr - 1) = '\0';
        sscanf(ptr_aux, " %s %ld", iext, &isize);
        *(bufferptr - 1) = ' ';

        if (getNumberOfDigits(isize) > 10 || strlen(iext) != 3)
        {
            printf("invalid\n");
            writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
            free(buffer);
            return 0;
        }

        reallocate(&buffersize, isize, &buffer, &bufferptr);

        // Creates image file and writes the data it's reading from the buffer to the file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/qimg.");
        strcat(path_aux, iext);
        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;
        ptr = bufferptr;
        readFull(connfd, &bufferptr, isize);
        writeFull(fd, ptr, isize);
        if (close(fd) < 0)
            return -1;

        // Reads '\n'
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            writeFull(connfd, "QUR OK\n", strlen("QUR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else if (*(bufferptr - n) == '0') // Has no image to read
    {
        // Reads '\n'
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            writeFull(connfd, "QUR OK\n", strlen("QUR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else
    {
        printf("failure\n");
        writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
        free(buffer);
        return 0;
    }
}

int answerSubmit(int connfd, char *buffer)
{
    int answer_count = 0;
    int auserid, n, fd;
    long asize, buffersize = BUFF_MAX;
    char *ptr, *bufferptr;
    char topic[F_MAX], question[F_MAX], pathname[P_MAX], path_aux[P_MAX], answer_name[F_MAX], anscount_str[4];

    bufferptr = &buffer[4];

    readUntil(connfd, &bufferptr, 4, ' ');

    // Scans command information
    *(bufferptr - 1) = '\0';
    sscanf(buffer, "ANS %d %s %s %ld", &auserid, topic, question, &asize);
    *(bufferptr - 1) = ' ';
    if (getNumberOfDigits(asize) > 10)
    {
        printf("invalid\n");
        writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
        free(buffer);
        return 0;
    }

    reallocate(&buffersize, asize, &buffer, &bufferptr);

    printf("User %d is trying to submit answer for question %s in topic %s... ", auserid, question, topic);

    findTopic(topic, pathname);
    findQuestion(question, pathname, NULL);
    answer_count = getAnswerCount(pathname);

    if (answer_count >= 99)
    {
        writeFull(connfd, "ANR FUL\n", strlen("ANR FUL\n"));
        free(buffer);
        return 0;
    }
    else
    {
        // Updates answer count file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/anscount.txt");
        if ((fd = open(path_aux, O_WRONLY | O_TRUNC)) < 0)
            return -1;
        answer_count++;
        sprintf(anscount_str, "%d\n", answer_count);
        writeFull(fd, anscount_str, strlen(anscount_str));
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
    ptr = bufferptr;
    readFull(connfd, &bufferptr, asize);
    writeFull(fd, ptr, asize);
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
        char iext[EXT_MAX];
        long isize;

        ptr = bufferptr;
        readUntil(connfd, &bufferptr, 3, ' ');

        // Scans file info
        *(bufferptr - 1) = '\0';
        sscanf(ptr, " %s %ld", iext, &isize);
        *(bufferptr - 1) = ' ';

        if (getNumberOfDigits(isize) > 10 || strlen(iext) != 3)
        {
            printf("invalid\n");
            writeFull(connfd, "QUR NOK\n", strlen("QUR NOK\n"));
            free(buffer);
            return 0;
        }

        reallocate(&buffersize, isize, &buffer, &bufferptr);

        // Creates image file and writes the data it's reading from the buffer to the file
        strcpy(path_aux, pathname);
        strcat(path_aux, "/");
        if (answer_count > 9)
            sprintf(answer_name, "%d_%d.%s", answer_count, auserid, iext);
        else
            sprintf(answer_name, "0%d_%d.%s", answer_count, auserid, iext);
        strcat(path_aux, answer_name);
        if ((fd = open(path_aux, O_WRONLY | O_CREAT, 0666)) < 0)
            return -1;
        ptr = bufferptr;
        readFull(connfd, &bufferptr, isize);
        writeFull(fd, ptr, isize);
        if (close(fd) < 0)
            return -1;

        // Reads '\n'
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            writeFull(connfd, "ANR OK\n", strlen("ANR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            writeFull(connfd, "ANR NOK\n", strlen("ANR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else if (*(bufferptr - n) == '0') // Has no image to read
    {
        // Reads '\n'
        if ((n = read(connfd, bufferptr, 1)) <= 0)
            return -1;
        if (*bufferptr == '\n')
        {
            printf("success\n");
            writeFull(connfd, "ANR OK\n", strlen("ANR OK\n"));
            free(buffer);
            return 0;
        }
        else
        {
            printf("failure\n");
            writeFull(connfd, "ANR NOK\n", strlen("ANR NOK\n"));
            free(buffer);
            return 0;
        }
    }
    else
    {
        printf("failure\n");
        writeFull(connfd, "ANR NOK\n", strlen("ANR NOK\n"));
        free(buffer);
        return 0;
    }
}

int receiveUDP(int udpfd, char *buffer, struct sockaddr_in *cliaddr, socklen_t *len)
{
    char request[8], response[BUFF_MAX];
    int id, n;

    if ((n = recvfrom(udpfd, buffer, 2048, 0, (struct sockaddr *)cliaddr, len)) == -1)
        return -1;
    buffer[n] = '\0'; // Appends a '\0' to the message so it can be used in strcmp
    sscanf(buffer, "%s", request);

    // register
    if (!strcmp(request, "REG"))
    {
        if (registerUser(buffer, &id, response, cliaddr) == -1)
            exit(1);
    }
    // topic_list
    else if (!strcmp(request, "LTP"))
    {
        if (topicList(response) == -1)
            exit(1);
    }
    // topic_propose
    else if (!strcmp(request, "PTP"))
    {
        if (topicPropose(buffer, &id, response) == -1)
            exit(1);
    }
    // question_list
    else if (!strcmp(request, "LQU"))
    {
        if (questionList(buffer, response) == -1)
            exit(1);
    }
    // error
    else
        strcpy(response, "ERR\n");

    return sendto(udpfd, response, strlen(response), 0, (struct sockaddr *)cliaddr, *len);
}

int receiveTCP(int connfd, struct sockaddr_in *cliaddr, socklen_t *len)
{
    char request[8];
    char *bufferptr, *buffer;

    if ((buffer = (char *)malloc(BUFF_MAX * sizeof(char))) == NULL)
        exit(1);
    bufferptr = &buffer[0];

    readUntil(connfd, &bufferptr, 1, ' ');
    buffer[3] = '\0';
    sscanf(buffer, "%s ", request);
    buffer[3] = ' ';

    // question_get
    if (!strcmp(request, "GQU"))
        return questionGet(connfd, buffer);
    // question_submit
    else if (!strcmp(request, "QUS"))
        return questionSubmit(connfd, buffer);
    // answer_submit
    else if (!strcmp(request, "ANS"))
        return answerSubmit(connfd, buffer);
    // error
    else
    {
        printf("Unexpected protocol message received\n");
        writeFull(connfd, "ERR\n", strlen("ERR\n"));
        free(buffer);
        return 0;
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd, udpfd, nready, maxfdp1, errcode;
    char buffer[BUFF_MAX], port[16];
    fd_set rset;
    struct sockaddr_in cliaddr;
    struct addrinfo hintstcp, hintsudp, *restcp, *resudp;
    socklen_t len;
    struct sigaction act, act2;
    pid_t childpid;
    DIR *dir;
    struct dirent *entry;

    memset(buffer, 0, sizeof buffer);

    // Initializes address info structs
    memset(&hintstcp, 0, sizeof hintstcp);
    hintstcp.ai_family = AF_INET;
    hintstcp.ai_socktype = SOCK_STREAM;
    hintstcp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    memset(&hintsudp, 0, sizeof hintsudp);
    hintsudp.ai_family = AF_INET;
    hintsudp.ai_socktype = SOCK_DGRAM;
    hintsudp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    // Sets up signal handlers for SIGPIPE and SIGCHLD
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
        exit(1);
    memset(&act2, 0, sizeof act);
    act2.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act2, NULL) == -1)
        exit(1);

    setPort(argc, argv, port);

    // Counts number of topics
    if ((dir = opendir("server/topics")) == NULL)
        exit(1);
    readdir(dir); // Skips directory .
    readdir(dir); // Skips directory ..
    while ((entry = readdir(dir)) != NULL)
        topic_amount++;
    closedir(dir);

    if ((errcode = getaddrinfo(NULL, port, &hintstcp, &restcp)) != 0)
        exit(1);
    if ((errcode = getaddrinfo(NULL, port, &hintsudp, &resudp)) != 0)
        exit(1);

    // Initializes TCP socket
    if ((listenfd = socket(restcp->ai_family, restcp->ai_socktype, restcp->ai_protocol)) == -1)
        exit(1);
    if (bind(listenfd, restcp->ai_addr, restcp->ai_addrlen) == -1)
        exit(1);
    if (listen(listenfd, 5) == -1)
        exit(1);

    // Initializes UDP socket
    if ((udpfd = socket(resudp->ai_family, resudp->ai_socktype, resudp->ai_protocol)) == -1)
        exit(1);
    if ((errcode = bind(udpfd, resudp->ai_addr, resudp->ai_addrlen)) == -1)
        exit(1);

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;

    while (1)
    {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        if ((nready = select(maxfdp1 + 1, &rset, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL)) <= 0)
            exit(1);

        // Ready to receive (TCP socket)
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
                close(listenfd);
                if (receiveTCP(connfd, &cliaddr, &len) == -1)
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
        // Ready to receive (UDP socket)
        if (FD_ISSET(udpfd, &rset))
        {
            len = sizeof(cliaddr);
            memset(buffer, 0, sizeof buffer);
            if (receiveUDP(udpfd, buffer, &cliaddr, &len) == -1)
                exit(1);
        }
    }
    freeaddrinfo(restcp);
    freeaddrinfo(resudp);
    close(listenfd);
    close(udpfd);
    exit(0);
}