#include <stdio.h>

/// Auxiliary data structure that serves as a wrapper to a socket file descriptor.
typedef struct Connection Connection;

/// @brief Sets up a TCP connection with a non-blocking socket.
/// @param addressName The address of the host to connect to
/// @param port The port of the host to connect to
/// @return The pointer to the connection on success, NULL/0 otherwise
Connection *tcp_init(char *addressName, unsigned int port);

/// @brief Reads the specified amount of bytes from the given connection. This function allocates memory.
/// @param connection The pointer to the connection to read bytes from
/// @param outBuffer The pointer to the buffer where the bytes are stored
/// @param outBufferCapacity The capacity of the supplied buffer
/// @return The amount of bytes read, or -1 on failure.
int tcp_read(Connection *connection, char *outBuffer, unsigned int outBufferCapacity);
/// @brief Writes the specified message to a given connection.
/// @param connection The pointer to the connection to which to send the message
/// @param message The pointer to the message to be sent
/// @param numBytes The number of bytes in the message
/// @return 0 on success, 1 if no bytes were sent, or 2 if the wrong number of bytes were sent
int tcp_write(Connection *connection, char *message, unsigned int numBytes);
/// @brief Closes a TCP connection. This function frees the memory allocated for the connection.
/// @param connection The pointer to the connection to be closed.
/// @return 0 on success, or 1 on failure
int tcp_close(Connection *connection);
