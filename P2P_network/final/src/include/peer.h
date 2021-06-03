#include <pthread.h>
#include "meta_file.h"
#include "proto.h"

#define MAX_NUM_FILES 10

typedef struct
{
    Meta_File *file;
    Peer_info *client_info;
    uint8_t client_piece_map[PIECE_MAP_BYTES];
    uint8_t swarm_piece_map[PIECE_MAP_BYTES];
    uint8_t num_peers;
    Peer_info peers[MAX_NUM_PEERS];
    pthread_mutex_t access;
    pthread_cond_t new_peers;
} File_State;

typedef struct
{
    File_State *pFS;
    int peer_idx;
} Contact_Info;

typedef struct
{
    File_State *pFS;
    int peer_sockfd;
} Service_Info;

typedef struct
{
    File_State *pFS;
    int tracker_sockfd;
    struct sockaddr_in tracker_addr;
} Tracker_Info;

void fileStateInit(File_State *pFS, Meta_File *file, Peer_info *client_info);
void contactInfoInit(Contact_Info *pCI, File_State *pFS, int peer_idx);
void serviceInfoInit(Service_Info *pSI, File_State *pFS, int peer_sockfd);
void trackerInfoInit(Tracker_Info *pTI, File_State *pFS, struct sockaddr_in tracker_addr);
int addPeer(File_State *pFS, Peer_info peer);
int removePeer(File_State *pFS, Peer_info *peer);
void updatePeers(File_State *pFS, int num_peers, Peer_info peers[MAX_NUM_PEERS], uint8_t swarm_piece_map[PIECE_MAP_BYTES]);
Peer_info *getPeer(File_State *pFS, int peer_index);
void setClientPiece(File_State *pFS, int piece_index);
void clearClientPiece(File_State *pFS, int piece_index);
int getClientPiece(File_State *pFS, uint8_t peer_piece_map[PIECE_MAP_BYTES]);
int checkPiece(uint8_t piece_map[PIECE_MAP_BYTES], int piece_index);
int checkPieceMap(File_State *pFS);
int swarmInterestingPieces(File_State *pFS);
void procTrackerResponse(File_State *pFS, Msg *pResponse);
void contactTracker(Tracker_Info *pTI);
void servePeer(Service_Info *pSI);
void contactPeer(Contact_Info *pCI);
void downloadFile(File_State *pFS);
void printFileState(File_State *pFS);