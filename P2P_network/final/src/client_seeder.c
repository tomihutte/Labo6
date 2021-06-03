#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "include/peer.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
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
    if (argc < 5) //command line argument check
    {
        fprintf(stderr, "usage %s client_port client_id i_filename o_filename\n", argv[0]);
        exit(0);
    }
    struct sockaddr_in tracker_addr = set_addr(AF_INET, (int)TRACKER_PORT, TRACKER_IP); //tracker addr
    struct sockaddr_in client_addr = set_addr(AF_INET, atoi(argv[1]), CLIENT_IP);       //client addr

    pthread_attr_t attr; //thr attributes for all threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    Peer_info *client_info = malloc(sizeof(Peer_info)); // client Peer_info set up
    uint8_t client_id[PEER_ID_BYTES];
    bzero(client_id, PEER_ID_BYTES);
    memcpy(client_id, (int *)argv[2], sizeof(argv[2]));
    setPeerInfo(client_info, client_id, ntohl(client_addr.sin_addr.s_addr), ntohs(client_addr.sin_port));

    File_State *pFS = malloc(sizeof(File_State)); // File_State set up
    Meta_File *pMF = malloc(sizeof(Meta_File));
    makeMetaFile(pMF, argv[3], PIECE_SIZE); //create meta file
    writeMetaFile(pMF, argv[4]);            // write meta file to given directory
    fileStateInit(pFS, pMF, client_info);
    for (int i = 0; i < fileNumPieces(pMF); i++) //client is a seeder, all pieces set
        setClientPiece(pFS, i);

    Tracker_Info *pTI = malloc(sizeof(Tracker_Info)); //communication with tracker
    trackerInfoInit(pTI, pFS, tracker_addr);
    pthread_t tracker_thr_id = 0;
    if (pthread_create(&tracker_thr_id, &attr, (void *)contactTracker, (void *)pTI) != 0)
    {
        perror("pthread_create:");
        exit(1);
    }

    printf("My info: IP:%s - Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    struct sockaddr_in peer_addr; //peer listen
    int peer_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(peer_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }
    listen(peer_sockfd, 5);
    socklen_t peerlen = sizeof(peer_addr);
    pthread_t thr_id = 0;
    int new_sockfd;
    while (1) //loop to handle peers communicating
    {
        Service_Info *pSI = malloc(sizeof(Service_Info));
        new_sockfd = accept(peer_sockfd, (struct sockaddr *)&peer_addr, &peerlen);
        if (new_sockfd < 0)
            error("ERROR on accept");
        serviceInfoInit(pSI, pFS, new_sockfd);
        if (pthread_create(&thr_id, &attr, (void *)servePeer, (void *)pSI) != 0)
        {
            perror("pthread_create:");
            exit(1);
        }
    }

    return 0;
}
