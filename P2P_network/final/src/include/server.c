#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "meta_file.h"
#include "server.h"

/* initializes client with its info and socket for thread to use */
void clientInit(Client_t *pC, struct sockaddr_in client_address, int socket_fd, File_Container_t *pFC)
{
    pC->address = client_address;
    pC->socket_fd = socket_fd;
    pC->pFC = pFC;
}
/* initializes file with its hash, num_peers and its seeder */
void fileInit(File_t *pF, uint8_t *file_hash, Peer_info seeder, uint8_t piece_map[PIECE_MAP_BYTES])
{
    memcpy(pF->file_hash, file_hash, SHA_DIGEST_LENGTH);
    pF->num_peers = 1;
    pF->peers[0] = seeder;
    if (gettimeofday(&pF->peers_last_update[0], NULL) == -1)
        perror("gettimeofday file init");
    memcpy(pF->piece_map, piece_map, PIECE_MAP_BYTES);
    int errnum;
    if ((errnum = pthread_mutex_init(&(pF->access), NULL)) != 0)
        printf("file mutex_init access: %s\n", strerror(errnum));
}
/* update file piece map with new piece map from new peers */
void updatePieceMap(File_t *pF, uint8_t piece_map[PIECE_MAP_BYTES])
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pF->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < PIECE_MAP_BYTES; i++)
    {
        pF->piece_map[i] |= piece_map[i];
    }
    if ((errnum = pthread_mutex_unlock(&pF->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
}
/* returns -1 if list of peers is full, 1 if peer is in list and replaces it with the new peer, 0 if succesfull*/
int addPeer(File_t *pF, Peer_info peer)
{
    int errnum;
    if (pF->num_peers == MAX_NUM_PEERS)
        return -1;
    if ((errnum = pthread_mutex_lock(&pF->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < pF->num_peers; i++)
    {
        if (memcmp(peer.peer_id, pF->peers[i].peer_id, PEER_ID_BYTES) == 0)
        {
            pF->peers[i] = peer;
            if (gettimeofday(&pF->peers_last_update[i], NULL) == -1)
                perror("gettimeofday add peer");
            if ((errnum = pthread_mutex_unlock(&pF->access)) != 0)
            {
                printf("mutex_unlock file access: %s", strerror(errnum));
                exit(1);
            }
            return 1;
        }
    }
    pF->peers[pF->num_peers] = peer;
    if (gettimeofday(&pF->peers_last_update[pF->num_peers], NULL) == -1)
        perror("gettimeofday add peer");
    pF->num_peers++;
    if ((errnum = pthread_mutex_unlock(&pF->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return 0;
}
/* returns -1 if peer not in list, 0 if successfull, clears piece_map completely, it will be updated with communications with peers*/
int removePeer(File_t *pF, Peer_info peer)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pF->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < pF->num_peers; i++)
    {
        if (memcmp(pF->peers[i].peer_id, peer.peer_id, SHA_DIGEST_LENGTH) == 0)
        {
            pF->peers[i] = pF->peers[pF->num_peers];
            pF->peers_last_update[i] = pF->peers_last_update[pF->num_peers];
            pF->num_peers--;
            bzero(pF->piece_map, PIECE_MAP_BYTES); // I cant tell wich pieces belong to which peer
            if ((errnum = pthread_mutex_unlock(&pF->access)) != 0)
            {
                printf("mutex_unlock file access: %s", strerror(errnum));
                exit(1);
            }
            return 0;
        }
    }
    if ((errnum = pthread_mutex_unlock(&pF->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return -1;
}
/* prints file information */
void printFile(File_t *pF)
{
    char hash_readable[SHA_DIGEST_LENGTH * 2];
    format_char_hexa(pF->file_hash, hash_readable, SHA_DIGEST_LENGTH);
    printf("Hash: %s\n", hash_readable);
    uint32_t peer_ip = ntohl(pF->peers[0].peer_ip);
    printf("SEEDER -> IP:%s - Port:%d - ID:%s\n", inet_ntoa(*(struct in_addr *)&peer_ip), ntohs(pF->peers[0].peer_port), pF->peers[0].peer_id);
}
/* initializes file container */
void fileContainerInit(File_Container_t *pFC)
{
    int i;
    for (i = 0; i < MAX_NUM_FILES; i++)
    {
        pFC->files[i] = NULL;
    }
    pFC->size = 0;
    int errnum;
    if ((errnum = pthread_mutex_init(&(pFC->access), NULL)) != 0)
        printf("container mutex_init access: %s\n", strerror(errnum));
}
/* frees al files from file container */
void fileContainerDestroy(File_Container_t *pFC)
{
    for (int i = 0; i < pFC->size; i++)
    {
        free(pFC->files[i]);
    }
}
/* returns File_t* if file is in the container, NULL if it isnt */
File_t *getFile(File_Container_t *pFC, uint8_t *file_hash)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFC->access)) != 0)
    {
        printf("mutex_lock container access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < pFC->size; i++)
    {
        if (compareFilesHash(pFC->files[i]->file_hash, file_hash) == 0)
        {
            File_t *ret = pFC->files[i];
            if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
            {
                printf("mutex_unlock container access: %s", strerror(errnum));
                exit(1);
            }
            return ret;
        }
    }
    if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
    {
        printf("mutex_unlock container access: %s", strerror(errnum));
        exit(1);
    }
    return NULL;
}
/* returns -1 if file container is full, 0 if successfull, if the file is already in container, adds all peers to the previous file and returns 1 */
int addFile(File_Container_t *pFC, File_t *pF)
{
    int errnum;
    File_t *prevF = getFile(pFC, pF->file_hash);
    if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
    {
        printf("mutex_unlock container access: %s", strerror(errnum));
        exit(1);
    }
    if (prevF == NULL)
    {
        if (pFC->size == MAX_NUM_FILES)
        {
            if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
            {
                printf("mutex_unlock container access: %s", strerror(errnum));
                exit(1);
            }
            return -1;
        }

        pFC->files[pFC->size] = pF;
        pFC->size++;
        if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
        {
            printf("mutex_unlock container access: %s", strerror(errnum));
            exit(1);
        }
        return 0;
    }
    else
    {
        for (int i = 0; i < pF->num_peers; i++)
        {
            if ((errnum = addPeer(prevF, pF->peers[i])) == -1) /* new file will always have only one peer */
                break;
            else if (errnum == 0)
                updatePieceMap(prevF, pF->piece_map); /* piece map of file is equal to  all new peers piece map (if file has more than one peer there is a problem here ) */
        }
    }
    if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
    {
        printf("mutex_unlock container access: %s", strerror(errnum));
        exit(1);
    }
    return 1;
}
/* returns 0 if successfull, -1 if file not in container */
int removeFile(File_Container_t *pFC, uint8_t *file_hash)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFC->access)) != 0)
    {
        printf("mutex_lock container access: %s", strerror(errnum));
        exit(1);
    }
    int i;
    for (i = 0; i < pFC->size; i++)
    {
        if (compareFilesHash(pFC->files[i]->file_hash, file_hash) == 0)
        {
            pFC->files[i] = pFC->files[pFC->size];
            pFC->size--;
            if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
            {
                printf("mutex_unlock container access: %s", strerror(errnum));
                exit(1);
            }
            return 0;
        }
    }
    if ((errnum = pthread_mutex_unlock(&pFC->access)) != 0)
    {
        printf("mutex_unlock container access: %s", strerror(errnum));
        exit(1);
    }
    return -1;
}
/* prints all files and file info in file container */
void printFiles(File_Container_t *pFC)
{
    for (int i = 0; i < pFC->size; i++)
    {
        printFile(pFC->files[i]);
    }
}
/* monitors all clients and drops ones who havent updated status for more tha MAX_NUM_INTERVALS*INTERVAL seconds */
void monitorClientUpdates(File_Container_t *pFC)
{
    struct timeval now, peer_update;
    double update_time;
    while (1)
    {
        for (int i = 0; i < pFC->size; i++)
        {
            for (int j = 0; j < pFC->files[i]->num_peers; j++)
            {
                if (gettimeofday(&now, NULL))
                    perror("gettimeofday monitor clients");
                peer_update = pFC->files[i]->peers_last_update[j];
                update_time = (now.tv_sec - peer_update.tv_sec) + (now.tv_usec - peer_update.tv_usec) / 1e6;
                if (update_time > MAX_NUM_INTERVALS * INTERVAL)
                {
                    printf("Removing peer %s \n", pFC->files[i]->peers[j].peer_id);
                    removePeer(pFC->files[i], pFC->files[i]->peers[j]);
                }
            }
        }
    }
    sleep(INTERVAL);
}
/* adds/updates file info received from peer to File Container, returns pointer
to file container requested by peer or NULL if tracker cant handle that file */
File_t *procTrackerRequest(Msg *pMSG, File_Container_t *pFC)
{
    File_t *pF = malloc(sizeof(File_t));
    fileInit(pF, pMSG->payload.tracker_request.file_hash, pMSG->payload.tracker_request.peer, pMSG->payload.tracker_request.piece_map);
    int err = addFile(pFC, pF);
    if (err == 1)
    {
        free(pF);                                                   /* if file is already in file container, its updated */
        pF = getFile(pFC, pMSG->payload.tracker_request.file_hash); /* change for previous file and discard the new */
    }
    else if (err == -1)
    {
        free(pF);
        pF = NULL;
    }
    return pF;
}
/* answers peer tracker_request by sending a tracker response with a list of peers for
file requested*/
void servePeer(Client_t *pClient)
{
    // char id_hexa[PEER_ID_BYTES * 2];
    // printf("Talking with client - IP:%s - Port: %d\n", inet_ntoa(pClient->address.sin_addr), ntohs(pClient->address.sin_port));
    Msg *pMSG_request = malloc(sizeof(Msg));
    Msg *pMSG_response = malloc(sizeof(Msg));
    int err;
    uint16_t tracker_interval = INTERVAL;
    while (1)
    {
        err = recvMsg(pClient->socket_fd, pMSG_request);
        if (err == 1)
        {
            if (pMSG_request->hdr.type != TRACKER_REQUEST)
                break;
            File_t *pF = procTrackerRequest(pMSG_request, pClient->pFC);
            if (pF == NULL)
                break;
            uint32_t ip = ntohl(pMSG_request->payload.tracker_request.peer.peer_ip);
            printf("Talking with client %s - IP:%s - Port: %d\n", pMSG_request->payload.tracker_request.peer.peer_id, inet_ntoa(*(struct in_addr *)&ip), ntohs(pMSG_request->payload.tracker_request.peer.peer_port));
            printf("I have %d peers for file requested\n", pF->num_peers);
            setTackerResponse(pMSG_response, tracker_interval, pF->num_peers, pF->piece_map, pF->peers);
            sendMsg(pClient->socket_fd, pMSG_response);
        }
        else if (err == 0)
        {
            printf("Socket closed\n");
            break;
        }
        else if (err == -1)
        {
            printf("Error receiving\n");
            break;
        }
    }
    free(pMSG_request);
    free(pMSG_response);
    close(pClient->socket_fd);
    free(pClient);
}
