#include "proto.h"
#include <pthread.h>

#define MAX_NUM_FILES 10
#define MAX_DATA_BYTE_SIZE 1024
#define INTERVAL 5
#define MAX_NUM_INTERVALS 3

typedef struct
{
    uint8_t file_hash[SHA_DIGEST_LENGTH];
    uint8_t num_peers;
    Peer_info peers[MAX_NUM_PEERS];
    struct timeval peers_last_update[MAX_NUM_PEERS];
    pthread_mutex_t access;
    uint8_t piece_map[PIECE_MAP_BYTES];
} File_t;

typedef struct
{
    File_t *files[MAX_NUM_FILES];
    int size;
    pthread_mutex_t access;
} File_Container_t;

typedef struct
{
    struct sockaddr_in address;
    int socket_fd;
    File_Container_t *pFC;
} Client_t;

void clientInit(Client_t *pC, struct sockaddr_in client_address, int socket_fd, File_Container_t *pFC);
void fileInit(File_t *pF, uint8_t *file_hash, Peer_info seeder, uint8_t piece_map[PIECE_MAP_BYTES]);
void updatePieceMap(File_t *pF, uint8_t piece_map[PIECE_MAP_BYTES]);
int addPeer(File_t *pF, Peer_info peer);
int removePeer(File_t *pF, Peer_info peer);
void printFile(File_t *pF);
void fileContainerInit(File_Container_t *pFC);
void fileContainerDestroy(File_Container_t *pFC);
File_t *getFile(File_Container_t *pFC, uint8_t *file_hash);
int addFile(File_Container_t *pFC, File_t *pF);
int removeFile(File_Container_t *pFC, uint8_t *file_hash);
void printFiles(File_Container_t *pFC);
void monitorClientUpdates(File_Container_t *pFC);
File_t *procTrackerRequest(Msg *pMSG, File_Container_t *pFC);
void servePeer(Client_t *pClient);