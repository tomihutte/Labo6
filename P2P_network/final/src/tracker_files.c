#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "include/server.h"

/* error handling */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/* returns addr_set with sin_family, sin_port and sin_addr fields */
struct sockaddr_in set_addr(sa_family_t family, in_port_t port, char *dir)
{
    struct sockaddr_in addr;
    // pongo a cero todo lo que hay en el server address
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = family;
    addr.sin_port = htons(port);
    inet_pton(family, dir, &addr.sin_addr);
    return addr;
}

int main(int argc, char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); //socket where tracker listens
    if (sockfd < 0)                               //AF_INET=ipv4 SOCK_STREAM=TCP
        error("ERROR opening socket");

    struct sockaddr_in tracker_addr = set_addr(AF_INET, (int)TRACKER_PORT, TRACKER_IP); //setting tracker addr
    if (bind(sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)       //socked binded to tracker_addr
        error("ERROR on binding");
    listen(sockfd, 5); //tracker will listen on socket

    pthread_attr_t attr; //thread attr
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t thr_id = 0; //thread id

    File_Container_t *pFC = malloc(sizeof(File_Container_t)); // File container
    fileContainerInit(pFC);

    if (pthread_create(&thr_id, &attr, (void *)monitorClientUpdates, (void *)pFC) != 0)
    {
        perror("pthread_create:");
        exit(1);
    }
    thr_id++;

    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        Client_t *pClient = malloc(sizeof(Client_t));
        clientInit(pClient, cli_addr, newsockfd, pFC);
        if (pthread_create(&thr_id, &attr, (void *)servePeer, (void *)pClient) != 0)
        {
            perror("pthread_create:");
            exit(1);
        }

        thr_id++;
    }
    close(sockfd);
    fileContainerDestroy(pFC);
    free(pFC);
    return 0;
}
