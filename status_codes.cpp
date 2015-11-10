#include <unistd.h>
#include "status_codes.h"

// Send out a status code to the specified player
bool SendStatus(int socket, StatusCode status)
{
    char * data = (char*)&status;
    size_t left = sizeof(status);
    ssize_t rc;

    while (left)
    {
        rc = write(socket, data + sizeof(status) - left, left);
        if(rc <= 0) return false;
        left -= rc;
    }

    return true;
}

// Blocks until it receives a status
bool ReceiveStatus(int socket, StatusCode * status)
{
    StatusCode ret;
    char *data = (char*)&ret;
    size_t left = sizeof(*status);
    ssize_t rc;

    while (left) {
        rc = read(socket, data + sizeof(ret) - left, left);
        if (rc <= 0) return false;
        left -= rc;
    }

    *status = ret;
    return true;
}