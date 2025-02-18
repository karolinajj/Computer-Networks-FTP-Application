#include "ftp.h"
#include "tcp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#define MAX_FIELD_SIZE 200
#define MAX_COMMAND_SIZE 300
#define BATCH_SIZE 512
#define FTP_CONNECTION_PORT 21
#define FTP_END_OF_LINE "\r\n"

static char user[MAX_FIELD_SIZE];
static char pass[MAX_FIELD_SIZE];
static char host[MAX_FIELD_SIZE];
static char file[MAX_FIELD_SIZE];

int splitURL(char *url)
{

    if (
        url[0] != 'f' ||
        url[1] != 't' ||
        url[2] != 'p' ||
        url[3] != ':' ||
        url[4] != '/' ||
        url[5] != '/')
    {
        printf("URL Parse Error: Wrong URL starting characters!\n");
        return 1;
    }

    unsigned int currentIndex = 6;

    for (unsigned int targetIndex = 0; currentIndex < MAX_FIELD_SIZE; targetIndex++, currentIndex++)
    {
        user[targetIndex] = url[currentIndex];
        if (url[currentIndex] == ':' || url[currentIndex] == '/' || url[currentIndex] == '\0')
        {
            user[targetIndex] = '\0';
            break;
        }
    }
    if (currentIndex >= MAX_FIELD_SIZE)
    {
        printf("URL Parse Error: Length exceeded!\n");
        return 1;
    }

    printf("URL Parse - read user: %s ?...\n", user);

    if (url[currentIndex] == '/')
    {
        strcpy(host, user);
        strcpy(user, "anonymous");
        strcpy(pass, "anonymous");
        printf("URL Parse: no user found, anonymous assumed, host set to: %s.\n", host);
    }
    else if (url[currentIndex] == ':')
    {
        printf("URL Parse: user interpreted successfully.\n");

        currentIndex++;
        for (unsigned int targetIndex = 0; currentIndex < MAX_FIELD_SIZE; targetIndex++, currentIndex++)
        {
            pass[targetIndex] = url[currentIndex];
            if (url[currentIndex] == '@' || url[currentIndex] == '/' || url[currentIndex] == '\0')
            {
                pass[targetIndex] = '\0';
                break;
            }
        }

        printf("URL Parse: read password: %s.\n", pass);

        if (currentIndex >= MAX_FIELD_SIZE)
        {
            printf("URL Parse Error: Length exceeded!\n");
            return 1;
        }

        if (url[currentIndex] != '@')
        {
            printf("URL Parse Error: Wrong password termination code %c!\n", url[currentIndex]);
            return 1;
        }

        currentIndex++;
        for (unsigned int targetIndex = 0; currentIndex < MAX_FIELD_SIZE; targetIndex++, currentIndex++)
        {
            host[targetIndex] = url[currentIndex];
            if (url[currentIndex] == '/' || url[currentIndex] == '\0')
            {
                host[targetIndex] = '\0';
                break;
            }
        }
        printf("URL Parse: read host %s.\n", host);

        if (url[currentIndex] != '/')
        {
            printf("URL Parse Error: Wrong host termination code %c\n", url[currentIndex]);
            return 1;
        }
    }
    else
    {
        printf("URL Parse Error: Wrong user end code %c\n", url[currentIndex]);
        return 1;
    }

    currentIndex++;

    for (unsigned int targetIndex = 0; currentIndex < MAX_FIELD_SIZE; targetIndex++, currentIndex++)
    {
        file[targetIndex] = url[currentIndex];
        if (url[currentIndex] == '\0')
        {
            file[targetIndex] = '\0';
            break;
        }
    }
    printf("URL Parse: read file %s.\n", file);

    return 0;
}

Connection *commandConnection;
Connection *downloadConnection;

enum MESSAGE_STATE
{
    READING_NUMBERS,
    LAST_LINE,
    NOT_LAST_LINE,
    NOT_LAST_LINE_FOUND_NEW_LINE
};

int get_response(Connection *connection, char **message)
{
    unsigned int mssgSize = 100, lastMessageIndex = 0;
    message[0] = (char *)malloc(mssgSize * sizeof(char));

    char t = 0;
    const char timeout = 3;

    printf("Reading FTP response:\n\n");
    char running = 1;
    enum MESSAGE_STATE state = READING_NUMBERS;
    while (running)
    {
        if (t >= timeout)
        {
            printf("Response Read Error: response timeout!\n");
            return 1;
        }
        t++;

        char buffer[BATCH_SIZE];
        int ret = tcp_read(connection, buffer, BATCH_SIZE);
        if (ret == -1)
        {
            printf("Response Read Error: TCP connection failed!\n");
            return 1;
        }
        if (ret)
        {
            t = 0;
            for (unsigned int i = 0; i < ret && running; i++)
            {

                message[0][lastMessageIndex] = buffer[i];
                lastMessageIndex++;
                if (lastMessageIndex >= mssgSize)
                {
                    mssgSize *= 2;
                    message[0] = realloc(message[0], mssgSize * sizeof(char));
                    if (!message[0])
                    {
                        fprintf(stderr, "Response Read Error: Memory allocation failed!\n");
                        return 1;
                    }
                }

                printf("%c", buffer[i]);

                switch (state)
                {
                case READING_NUMBERS:
                    if (buffer[i] == ' ')
                    {
                        state = LAST_LINE;
                    }
                    else if (buffer[i] == '-')
                    {
                        state = NOT_LAST_LINE;
                    }
                    else if (buffer[i] == '\n')
                    {
                        running = 0;
                    }
                    else if (buffer[i] > '9' && buffer[i] < '0')
                    {
                        return -1;
                    }
                    break;
                case LAST_LINE:
                    if (buffer[i]=='\n')
                    {
                        running=0;
                    }

                    break;
                case NOT_LAST_LINE:
                
                    if (buffer[i]=='\n')
                    {
                        state=NOT_LAST_LINE_FOUND_NEW_LINE;
                    }
                    break;
                case NOT_LAST_LINE_FOUND_NEW_LINE:
                    if (buffer[i]>='0'&&buffer[i]<='9')
                    {
                        state=READING_NUMBERS;
                    }else{
                        state=NOT_LAST_LINE;
                    }
                    break;
                }
            }
        }
    }
    printf("\n");
    return 0;
}

int interpret_pasv_response(char *message, unsigned int *port, char *serverCode)
{
    int h1, h2, h3, h4, p1, p2;

    char *start = strrchr(message, '(');
    char *end = strrchr(message, ')');

    if (start == NULL || end == NULL || start > end)
    {
        printf("PASV: response has a wrong format.\n");
        return 1;
    }

    if (sscanf(start + 1, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6)
    {
        printf("PASV Error: couldn't parse the response.\n");
        return 1;
    }

    sprintf(serverCode, "%u.%u.%u.%u", h1, h2, h3, h4);
    *port = (p1 * 256) + p2;

    return 0;
}

enum SpecialState
{
    STATE_NONE = 0,
    STATE_LOGIN = 1,
    STATE_RETR = 2,
};

int check_is_error_code(char *message, char specialstate)
{
    return message[0] != '2' && !(specialstate == STATE_LOGIN && message[0] == '3') && !(specialstate == STATE_RETR && message[0] == '1');
}

int ftp_init(char *url)
{

    if (splitURL(url))
    {
        printf("Init Error: invalid URL!\n");
        return 1;
    }
    printf("Init: URL parsed successfully!\n");

    char *message;

    if (!(commandConnection = tcp_init(host, FTP_CONNECTION_PORT)))
    {
        printf("Init Error: Couldn't connect to port!\n");
        return 1;
    }
    if (
        get_response(commandConnection, &message))
    {
        printf("Init Error: Couldn't get response!\n");
        return 1;
    }

    if (check_is_error_code(message, 0))
    {

        printf("Init Error: Wrong response code!\n");
        return 1;
    }
    free(message);
    printf("Int: Inserting user.\n");

    char command[MAX_FIELD_SIZE] = "USER ";
    strcat(command, user);
    strcat(command, FTP_END_OF_LINE);
    if (tcp_write(commandConnection, command, strlen(command)))
    {
        printf("Init Error: couldn't write USER.\n");
        return 1;
    }
    if (
        get_response(commandConnection, &message))
    {
        printf("Init Error: Couldn't get response!\n");
        return 1;
    }

    if (check_is_error_code(message, 1))
    {

        printf("Init Error: Wrong response code!\n");
        return 1;
    }
    free(message);
    printf("Init: User inserted.\nInit: Sending password.\n");

    strcpy(command, "PASS ");
    strcat(command, pass);
    strcat(command, FTP_END_OF_LINE);
    printf("%s", command);
    if (tcp_write(commandConnection, command, strlen(command)))
    {
        printf("Init Error: couldn't write PASS.\n");
        return 1;
    }

    if (
        get_response(commandConnection, &message))
    {
        printf("Init Error: Couldn't get response!\n");
        return 1;
    }

    if (check_is_error_code(message, 0))
    {

        printf("Init Error: Wrong response code!\n");
        return 1;
    }
    free(message);

    printf("Init: Password inserted.\n Init: Sending PASV\n");

    strcpy(command, "PASV" FTP_END_OF_LINE);

    if (tcp_write(commandConnection, command, strlen(command)))
    {
        printf("Init Error: couldn't write PASV.\n");
        return 1;
    }

    unsigned int otherPort = 0;

    if (
        get_response(commandConnection, &message))
    {
        printf("Init Error: Couldn't get response!\n");
        return 1;
    }

    char secondSocketServerCode[MAX_FIELD_SIZE];
    if (interpret_pasv_response(message, &otherPort, secondSocketServerCode))
    {

        printf("Init Error: Couldn't parse PASSV response!\n");
        return 1;
    }
    free(message);
    printf("Init: Password inserted.\nInit: Opening download socket %d, on server %s!\n", otherPort, secondSocketServerCode);

    if (!(downloadConnection = tcp_init(secondSocketServerCode, otherPort)))
    {
        printf("Init Error: invalid URL!\n");
        return 1;
    }

    printf("Init: Opened download connection!\n");
    return 0;
}

int ftp_download_file(char *filename)
{
    printf("Download: Writing RETR!\n");
    char command[MAX_FIELD_SIZE] = "RETR ";

    strcat(command, file);
    strcat(command, FTP_END_OF_LINE);
    printf("command: %s", command);
    if (tcp_write(commandConnection, command, strlen(command)))
    {
        printf("Download Error: couldn't write RETR.\n");
        return 1;
    }

    char *message;
    if (
        get_response(commandConnection, &message))
    {
        printf("Download Error: Couldn't get response!\n");
        return 1;
    }

    if (check_is_error_code(message, STATE_RETR))
    {

        printf("Download Error: Wrong response code!\n");
        return 1;
    }
    free(message);

    printf("Download: RETR Successful.\nOpening file.\n");
    FILE *f = fopen(filename, "w");

    if (f == NULL)
    {
        printf("Download Error: couldn't open file locally.\n");
        return 1;
    }

    printf("Download: Starting download loop\n");

    while (downloadConnection)
    {

        char buffer[BATCH_SIZE];
        int ret = !downloadConnection ? 0 : tcp_read(downloadConnection, buffer, BATCH_SIZE);
        if (ret < 0)
        {
            printf("Download Error: TCP connection failed!\n");
            return 1;
        }
        else if (ret)
        {
            printf(".");

            if (fwrite(buffer, 1 /*size in bytes of element, which is a byte, therefore it's 1*/, ret, f) < ret)
            {
                printf("Download Error: failed to copy data to file!\n");
                return 1;
            }
        }
        else if (ret == 0)
        {
            printf("\nDownload: Data connection EOF reached!\n");
            printf("Download: Terminating file transfer!\n");
            tcp_close(downloadConnection);
            downloadConnection = 0;
        }
    }
    printf("Download: Finished download.\n");

    fclose(f);

    return 0;
}

int ftp_close()
{
    char command[MAX_FIELD_SIZE];
    char *message;

    printf("Close: Reading end-of-transmission message.\n");
    if (
        get_response(commandConnection, &message))
    {
        printf("Close Error: Couldn't get response!\n");
        return 1;
    }

    if (check_is_error_code(message, 0))
    {

        printf("Close Error: Wrong response code!\n");
        return 1;
    }
    free(message);

    printf("Close: Sending QUIT command.\n");
    strcpy(command, "QUIT" FTP_END_OF_LINE);
    if (tcp_write(commandConnection, command, strlen(command)))
    {
        printf("Close Error: couldn't write RETR.\n");
        return 1;
    }

    if (
        get_response(commandConnection, &message))
    {
        printf("Close Error: Couldn't get response!\n");
        return 1;
    }

    if (check_is_error_code(message, 0))
    {

        printf("Close Error: Wrong response code!\n");
        return 1;
    }

    printf("Close: terminated command connection successfully.\n");
    return tcp_close(commandConnection);
}