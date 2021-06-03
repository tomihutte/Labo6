#include "proto.h"
#include <errno.h>

// sends message stored in lineal buffer, returns 0 if socket
// is closed, -1 in case of error and 1 if everything is okay
int sendMsg(int sockfd, const Msg *msg)
{
    int toSend = ntohl(msg->hdr.payload_size) + sizeof(Header); // total bytes to send
    int sent;                                                   //
    uint8_t *ptr = (uint8_t *)msg;                              // pointer to data

    while (toSend) // while there is still something to send
    {
        sent = send(sockfd, ptr, toSend, 0);
        if ((sent == -1 && errno != EINTR) || sent == 0)
            return sent;
        toSend -= sent; // there is less to send
        ptr += sent;    // stand in next info to be sent
    }
    return 1;
}

// recieves and stores a message, returns 1 if everything is ok, -1 if theres an error
// and 0 if the socket is closed
int recvMsg(int sockfd, Msg *msg)
{
    int toRecv = sizeof(Header); // first the fixed size header is received
    int recvd;
    uint8_t *ptr = (uint8_t *)&msg->hdr; //pointer to data storage
    int headerRecvd = 0;                 // flag to check if already received header

    while (toRecv)
    {
        recvd = recv(sockfd, ptr, toRecv, 0);
        if ((recvd == -1 && errno != EINTR) || recvd == 0)
            return recvd;
        toRecv -= recvd;
        ptr += recvd;
        if (toRecv == 0 && headerRecvd == 0) // when header is fully received
        {
            headerRecvd = 1;                       // already received header
            ptr = (uint8_t *)&msg->payload;        // move storage to payload (necesario?)
            toRecv = ntohl(msg->hdr.payload_size); // set amount of data to be received as payload size specified by header
        }
    }
    return 1;
}
