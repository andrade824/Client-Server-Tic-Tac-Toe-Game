//
// Created by devon on 5/7/15.
//

#include <unistd.h>
#include "sockets.h"

/**
 * Send out an int to the specified player
 *
 * @param socket    The socket to send the int to
 * @param number    The integer to send out
 *
 * @return  True if the socket is still open, false if the socket was disconnected
 */
bool SendInt(int socket, int number)
{
    char * data = (char*)&number;
    size_t left = sizeof(number);
    ssize_t rc;

    while (left)
    {
        rc = write(socket, data + sizeof(number) - left, left);
        if(rc <= 0) return false;
        left -= rc;
    }

    return true;
}

/**
 * Blocks until it receives an int
 *
 * @param socket    The socket to send the int to
 * @param number    The integer that was received
 *
 * @return  True if the socket is still open, false if the socket was disconnected
 */
bool ReceiveInt(int socket, int * number)
{
    int ret;
    char *data = (char*)&ret;
    size_t left = sizeof(*number);
    ssize_t rc;

    while (left) {
        rc = read(socket, data + sizeof(ret) - left, left);
        if (rc <= 0) return false;
        left -= rc;
    }

    *number = ret;
    return true;
}