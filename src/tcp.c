#include "tcp.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
typedef struct Connection
{
    char *addressName;
    int port;
    int sockfd;
} Connection;

#define RANDOM_LONG_ENOUGH_NUMBER 200

// initializes a connection to a server by creating a socket
Connection *tcp_init(char *addressName, unsigned int port)
{
    Connection *connection = (Connection *)malloc(sizeof(Connection));
    if (!connection)
    {
        perror("TCP Error - malloc()! ");
        return 0;
    }

    char portStr[10];
    snprintf(portStr, sizeof(portStr), "%d", port);

    struct addrinfo hints, *a;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(addressName, portStr, &hints, &a) != 0)
    {
        perror("TCP Error - getaddrinfo()! ");
        free(connection);
        return 0;
    }

    connection->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connection->sockfd < 0)
    {
        perror("TCP Error: socket()! ");
        freeaddrinfo(a);
        free(connection);
        return 0;
    }
    printf("Socket opened!\n");

    int flags = fcntl(connection->sockfd, F_GETFL, 0); // non blocking socket
    if (flags == -1 || fcntl(connection->sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("TCP Error: fcntl! ");
        close(connection->sockfd);
        freeaddrinfo(a);
        free(connection);
        return 0;
    }

    // start of connecting
    int status = connect(connection->sockfd, a->ai_addr, a->ai_addrlen);
    if (status < 0 && errno != EINPROGRESS)
    {
        perror("TCP Error: connect()!");
        close(connection->sockfd);
        freeaddrinfo(a);
        free(connection);
        return 0;
    }

    if (status != 0) // connection in progress
    {
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(connection->sockfd, &writefds);

        status = select(connection->sockfd + 1, NULL, &writefds, NULL, &timeout);
        if (status <= 0)
        {
            if (status == 0)
            {
                fprintf(stderr, "TCP Error: connection timeout!\n");
            }
            else
            {
                perror("TCP Error - select() failed!");
            }
            close(connection->sockfd);
            freeaddrinfo(a);
            free(connection);
            return 0;
        }

        // checking if the socket is writable
        int optval;
        socklen_t optlen = sizeof(optval);
        if (getsockopt(connection->sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0 || optval != 0)
        {
            fprintf(stderr, "TCP Error: connect() failed with error %d\n", optval);
            close(connection->sockfd);
            freeaddrinfo(a);
            free(connection);
            return 0;
        }
    }

    printf("Connection established!\n");

    freeaddrinfo(a);

    connection->addressName = strdup(addressName); // returns NULL in case of an error,
    if (!connection->addressName)
    {
        perror("TCP Error - strdup()!");
        close(connection->sockfd);
        free(connection);
        return 0;
    }
    connection->port = port;

    return connection;
}

// sends data over the established TCP connection
int tcp_write(Connection *connection, char *message, unsigned int numBytes)
{
    int bytes = write(connection->sockfd, message, numBytes);
    if (bytes != numBytes)
    {
        if (bytes == 0)
        {
            perror("TCP Error - write()! ");
            return 1;
        }
        printf("TCP Error: Couldn't write all bytes!");
        return 2;
    }

    return 0;
}

// reads data from the established TCP connection into a buffer
int tcp_read(Connection *connection, char *outBuffer, unsigned int outBufferCapacity)
{

    if (connection == NULL || outBuffer == NULL || outBufferCapacity <= 0)
    {
        printf("TCP Error: wrong read parameters!\n");
        return -1; // invalid input
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(connection->sockfd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int status = select(connection->sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (status > 0)
    {
        ssize_t bytes = read(connection->sockfd, outBuffer, outBufferCapacity);
        if (bytes < 0)
        {
            perror("TCP Error: read()! ");
        }
        else
        {
        }
        return bytes;
    }
    else if (status == 0)
    {
        printf("TCP Failure: Timeout waiting for data!\n");
    }
    else
    {
        perror("TCP Error: select()! ");
        return -1;
    }
    return 0;
}

int tcp_close(Connection *connection)
{
    if (close(connection->sockfd) < 0)
    {
        free(connection->addressName);
        free(connection);
        perror("TCP Error: close()! ");
        return 1;
    }
    printf("Socket closed!\n");
    free(connection->addressName);
    free(connection);
    return 0;
}