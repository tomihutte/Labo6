#include "peer.h"

/* initializes File_State for a Meta_File */
void fileStateInit(File_State *pFS, Meta_File *file, Peer_info *client_info)
{
    pFS->file = file;
    bzero(pFS->client_piece_map, PIECE_MAP_BYTES);
    bzero(pFS->swarm_piece_map, PIECE_MAP_BYTES);
    pFS->num_peers = 0;
    pFS->client_info = client_info;
    int errnum;
    if ((errnum = pthread_mutex_init(&(pFS->access), NULL)) != 0)
        printf("file mutex_init access: %s\n", strerror(errnum));
    if ((errnum = pthread_cond_init(&pFS->new_peers, NULL)) != 0)
        printf("file cond_init access: %s\n", strerror(errnum));
}
/* initializes Contact Info for a given peer to work with it*/
void contactInfoInit(Contact_Info *pCI, File_State *pFS, int peer_idx)
{
    pCI->pFS = pFS;
    pCI->peer_idx = peer_idx;
}
/* initializes Service Info to serve a peer */
void serviceInfoInit(Service_Info *pSI, File_State *pFS, int peer_sockfd)
{
    pSI->pFS = pFS;
    pSI->peer_sockfd = peer_sockfd;
}
/* initializes Tracker Info to communicate with tracker */
void trackerInfoInit(Tracker_Info *pTI, File_State *pFS, struct sockaddr_in tracker_addr)
{
    pTI->pFS = pFS;
    pTI->tracker_addr = tracker_addr;
}
/* adds a peer to the file state, returns 0 if successfull, -1  if list of 
peers is full and 1 if peer is already in the list*/
int addPeer(File_State *pFS, Peer_info peer)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < pFS->num_peers; i++)
    {
        if (memcmp(pFS->peers[i].peer_id, peer.peer_id, PEER_ID_BYTES) == 0)
        {
            if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
            {
                printf("mutex_unlock file access: %s", strerror(errnum));
                exit(1);
            }
            return 1;
        }
    }

    if (pFS->num_peers == MAX_NUM_PEERS)
    {
        if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
        {
            printf("mutex_unlock file access: %s", strerror(errnum));
            exit(1);
        }
        return -1;
    }
    pFS->peers[pFS->num_peers] = peer;
    pFS->num_peers++;
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return 0;
}
/* removes Peer from peer list, returns 0 if successful, 1 if peer not in list */
int removePeer(File_State *pFS, Peer_info *peer)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < pFS->num_peers; i++)
    {
        if (memcmp(pFS->peers[i].peer_id, peer->peer_id, PEER_ID_BYTES) == 0)
        {

            pFS->peers[i] = pFS->peers[pFS->num_peers - 1];
            pFS->num_peers--;
            bzero(pFS->swarm_piece_map, PIECE_MAP_BYTES);
            if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
            {
                printf("mutex_unlock file access: %s", strerror(errnum));
                exit(1);
            }
            return 0;
        }
    }
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return 1;
}
/* replaces peer list with new peer list */
void updatePeers(File_State *pFS, int num_peers, Peer_info peers[MAX_NUM_PEERS], uint8_t swarm_piece_map[PIECE_MAP_BYTES])
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    pFS->num_peers = num_peers;
    memcpy(pFS->peers, peers, sizeof(Peer_info) * num_peers);
    memcpy(pFS->swarm_piece_map, swarm_piece_map, PIECE_MAP_BYTES);
    if ((errnum = pthread_cond_signal(&pFS->new_peers)) != 0)
    {
        printf("cond_signal get: %s\n", strerror(errnum));
        exit(1);
    }
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
}
/* returns the contact information (Peer_info*) of a given peer, if peer_index is
invald, returns NULL */
Peer_info *getPeer(File_State *pFS, int peer_index)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    if (peer_index >= pFS->num_peers)
    {
        return NULL;
    }
    Peer_info *ret = &pFS->peers[peer_index];
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return ret;
}
// set piece of bitmap as already downloaded (1)
void setClientPiece(File_State *pFS, int piece_index)
{
    int piece_group = piece_index / PIECE_MAP_STRUCT_SIZE;
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    pFS->client_piece_map[piece_group] |= 1 << (piece_index % PIECE_MAP_STRUCT_SIZE);
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
}
// set piece of bitmap as need to download (0)
void clearClientPiece(File_State *pFS, int piece_index)
{
    int piece_group = piece_index / PIECE_MAP_STRUCT_SIZE;
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    pFS->client_piece_map[piece_group] &= ~(1 << (piece_index % PIECE_MAP_STRUCT_SIZE));
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
}
/* returns the index of the first missing piece of the client that the peer has
and sets it as downloaded if there are no missing pieces returns -1 */
int getClientPiece(File_State *pFS, uint8_t peer_piece_map[PIECE_MAP_BYTES])
{
    int num_pieces = fileNumPieces(pFS->file);
    int piece_group;
    int errnum;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < num_pieces; i++)
    {
        if (checkPiece(peer_piece_map, i) == 0) //checking if peer has piece
            continue;

        piece_group = i / PIECE_MAP_STRUCT_SIZE;
        if (((pFS->client_piece_map[piece_group]) & ((1 << (i % PIECE_MAP_STRUCT_SIZE)))) == 0)
        {
            pFS->client_piece_map[piece_group] |= 1 << (i % PIECE_MAP_STRUCT_SIZE);
            if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
            {
                printf("mutex_unlock file access: %s", strerror(errnum));
                exit(1);
            }
            return i;
        }
    }
    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return -1;
}
/* returns piece state (1 if set as downloaded, 0 if set as  not downloaded) - 
works on any piece map, no File State needed*/
int checkPiece(uint8_t piece_map[PIECE_MAP_BYTES], int piece_index)
{
    int piece_group = piece_index / PIECE_MAP_STRUCT_SIZE;
    return piece_map[piece_group] & (1 << (piece_index % PIECE_MAP_STRUCT_SIZE));
}
/* returns 1 if piece map is completed, 0 if not */
int checkPieceMap(File_State *pFS)
{
    int num_pieces = fileNumPieces(pFS->file);
    int errnum, ret = 1;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < num_pieces; i++)
    {
        if (checkPiece(pFS->client_piece_map, i) == 0)
        {
            ret = 0;
            break;
        }
    }

    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
    return ret;
}
/* check if there are any pieces in swarm that client does not have */
int swarmInterestingPieces(File_State *pFS)
{
    int num_pieces = fileNumPieces(pFS->file);
    for (int i = 0; i < num_pieces; i++)
    {
        if ((checkPiece(pFS->client_piece_map, i) == 0) && (checkPiece(pFS->swarm_piece_map, i) == 1))
            return 1;
    }
    return 0;
}
/* processes tracker response, adding all new peers to de File State */
void procTrackerResponse(File_State *pFS, Msg *pResponse)
{
    if (pResponse->hdr.type != TRACKER_RESPONSE)
        return;
    int num_peers = pResponse->payload.tracker_response.num_peers;
    updatePeers(pFS, num_peers, pResponse->payload.tracker_response.peers, pResponse->payload.tracker_response.piece_map);
}
/* Starts contact to tracker and repeats contact acording to tracker interval */
void contactTracker(Tracker_Info *pTI)
{
    File_State *pFS = pTI->pFS;
    Meta_File *pMF = pFS->file;
    Msg *pRequest = malloc(sizeof(Msg));
    int tracker_sockfd, interval;
    while (1)
    {
        tracker_sockfd = socket(AF_INET, SOCK_STREAM, 0); //tracker socket
        if (tracker_sockfd < 0)
        {
            perror("ERROR opening socket");
            exit(1);
        }
        setTackerRequest(pRequest, pMF->file_hash, *pFS->client_info, pFS->client_piece_map);
        if (connect(tracker_sockfd, (struct sockaddr *)&pTI->tracker_addr, sizeof(pTI->tracker_addr)) < 0)
        {
            perror("ERROR connecting");
            exit(1);
        }
        // printf("About to send info to tracker\n");
        while (sendMsg(tracker_sockfd, pRequest) != 1)
            continue;
        // printf("Uploaded info to tracker\n");
        Msg *pResponse = malloc(sizeof(Msg));
        recvMsg(tracker_sockfd, pResponse);
        close(tracker_sockfd);
        procTrackerResponse(pFS, pResponse);
        interval = ntohs(pResponse->payload.tracker_response.interval);
        sleep(interval);
    }
}
/* services peer, sending all pieces the peer requests  */
void servePeer(Service_Info *pSI)
{
    int peer_sockfd = pSI->peer_sockfd;
    File_State *pFS = pSI->pFS;
    Meta_File *pMF = pFS->file;
    // printf("Talking with peer\n");

    /* preparo el handshake */
    Msg *pHandshake = malloc(sizeof(Msg));
    int handshake_error = 0;
    if (recvMsg(peer_sockfd, pHandshake) <= 0)
        handshake_error = 1;
    if ((handshake_error == 0) && (pHandshake->hdr.type != PEER_HANDSHAKE))
        // printf("Expected PEER_HANDSHAKE Msg\n");
        handshake_error = 1;
    if ((handshake_error == 0) && (memcmp(pHandshake->payload.peer_handshake.peer_id, pFS->client_info->peer_id, PEER_ID_BYTES) == 0))
        // printf("Peer has same ID\n");
        handshake_error = 1;
    if ((handshake_error == 0) && (memcmp(pHandshake->payload.peer_handshake.file_hash, pMF->file_hash, SHA_DIGEST_LENGTH) != 0))
        // printf("Peer file hash doesn't match\n");
        handshake_error = 1;
    if ((handshake_error == 0) && (memcmp(pHandshake->payload.peer_handshake.piece_map, pFS->client_piece_map, PIECE_MAP_BYTES) == 0))
    {
        // printf("Peer has same pieces\n");
        handshake_error = 1;
        bzero(pHandshake, sizeof(Msg));
        setPeerHandshake(pHandshake, pMF->file_hash, pFS->client_info->peer_id, pFS->client_piece_map);
        sendMsg(peer_sockfd, pHandshake);
    }
    if (handshake_error == 1)
    {
        close(peer_sockfd);
        free(pHandshake);
        free(pSI);
        return;
    }
    bzero(pHandshake, sizeof(Msg));
    setPeerHandshake(pHandshake, pMF->file_hash, pFS->client_info->peer_id, pFS->client_piece_map);
    if (sendMsg(peer_sockfd, pHandshake) <= 0)
    {
        close(peer_sockfd);
        free(pHandshake);
        return;
    }
    free(pHandshake);
    // printf("Finished handshake\n");
    /* abro el archivo que voy a enviar */
    mode_t mask = umask(0000);
    int fd = open((char *)pMF->file_name, O_CREAT | O_RDONLY, 0777);
    if (fd < 0)
    {
        perror("opening file");
        exit(1);
    }
    uint32_t piece, block_offset;
    uint8_t data[FIXED_BLOCK_SIZE];
    Msg *pRequest = malloc(sizeof(Msg));
    Msg *pPiece = malloc(sizeof(Msg));
    int num_pieces = fileNumPieces(pMF);
    int num_blocks, offset, piece_size, block_size;

    while (recvMsg(peer_sockfd, pRequest) > 0)
    {
        if (pRequest->payload.peer_message.message_id != REQUEST)
            break;

        piece = ntohl(pRequest->payload.peer_message.payload.request.piece_index);
        if (checkPiece(pFS->client_piece_map, piece) == 0)
        {
            // printf("I dont have the piece requested\n");
            setPeerPiece(pPiece, 0, -1, -1, data);
            if (sendMsg(peer_sockfd, pPiece) <= 0)
                break;
            continue;
        }

        block_offset = ntohl(pRequest->payload.peer_message.payload.request.block_offset);
        if ((piece == num_pieces - 1))
            piece_size = (pMF->file_size % pMF->piece_size) == 0 ? pMF->piece_size : pMF->file_size % pMF->piece_size;
        else
            piece_size = pMF->piece_size;
        num_blocks = piece_size / FIXED_BLOCK_SIZE + (piece_size % FIXED_BLOCK_SIZE != 0);
        if (block_offset == num_blocks - 1)
            block_size = (piece_size % FIXED_BLOCK_SIZE) == 0 ? FIXED_BLOCK_SIZE : piece_size % FIXED_BLOCK_SIZE;
        else
            block_size = FIXED_BLOCK_SIZE;
        offset = piece * pMF->piece_size + block_offset * FIXED_BLOCK_SIZE;
        if (lseek(fd, offset, SEEK_SET) < 0)
        {
            perror("seek write file");
            break;
        }
        bzero(pPiece, sizeof(Msg));
        read(fd, data, block_size);
        setPeerPiece(pPiece, block_size, piece, block_offset, data);
        // printf("Sending piece %d - block %d\n", piece, block_offset);
        sleep(1); // espero para enviar, si no se baja todo muy rapido y no puedo ver al correr las cosas
        if (sendMsg(peer_sockfd, pPiece) <= 0)
            break;
        bzero(pRequest, sizeof(Msg));
    }
    free(pPiece);
    free(pRequest);
    free(pSI);
    close(peer_sockfd);
    close(fd);
    umask(mask);
}
/* contacts peer and request for all mising pieces to it */
void contactPeer(Contact_Info *pCI)
{
    File_State *pFS = pCI->pFS;
    Meta_File *pMF = pFS->file;
    /* preparo la estructura del seeder para establecer comunicación */
    int peer_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (peer_sockfd < 0)
    {
        // perror("ERROR opening socket");
        exit(-1);
    }
    struct sockaddr_in peer_addr;                  // estructura del seeder
    Peer_info *peer = getPeer(pFS, pCI->peer_idx); // datos del seeder
    bzero((char *)&peer_addr, sizeof(peer_addr));  // inicializo la estructura
    peer_addr.sin_family = AF_INET;                // defino ip y puerto
    peer_addr.sin_addr.s_addr = peer->peer_ip;
    peer_addr.sin_port = peer->peer_port;
    // printf("Seeder info - IP:%s - Port: %d\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));

    /* me conecto al seeder */
    if (connect(peer_sockfd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0)
    {
        // perror("ERROR connecting");
        removePeer(pFS, peer);
        close(peer_sockfd);
        free(pCI);
        return;
    }

    /* preparo el handshake */
    int close_comm = 0;
    Msg *pHandshake = malloc(sizeof(Msg));
    setPeerHandshake(pHandshake, pMF->file_hash, pFS->client_info->peer_id, pFS->client_piece_map);
    /* envio el handshake */
    if (sendMsg(peer_sockfd, pHandshake) < 0)
        close_comm = 1;
    bzero(pHandshake, sizeof(Msg));
    if ((close_comm == 0) && (recvMsg(peer_sockfd, pHandshake) <= 0))
    {
        close_comm = 1;
    }
    if ((close_comm == 0) && (pHandshake->hdr.type != PEER_HANDSHAKE))
    {
        // printf("Expected PEER_HANDSHAKE msg\n");
        close_comm = 1;
    }
    if ((close_comm == 0) && (memcmp(pHandshake->payload.peer_handshake.peer_id, pFS->client_info->peer_id, PEER_ID_BYTES) == 0))
    {
        // printf("Peer has same ID\n");
        close_comm = 1;
    }
    if ((close_comm == 0) && (memcmp(pHandshake->payload.peer_handshake.file_hash, pMF->file_hash, SHA_DIGEST_LENGTH) != 0))
    {
        // printf("Peer file hash doesn't match\n");
        close_comm = 1;
    }
    if ((close_comm == 0) && (memcmp(pHandshake->payload.peer_handshake.piece_map, pFS->client_piece_map, PIECE_MAP_BYTES) == 0))
    {
        // printf("Peer has same pieces\n");
        close_comm = 1;
    }
    if (close_comm == 1)
    {
        close(peer_sockfd);
        free(pHandshake);
        free(pCI);
        return;
    }
    uint8_t peer_piece_map[PIECE_MAP_BYTES];
    memcpy(peer_piece_map, pHandshake->payload.peer_handshake.piece_map, PIECE_MAP_BYTES);
    free(pHandshake);
    // printf("Finished Handshake with seeder\n");

    /* creo el archivo que voy a descargar */
    mode_t mask = umask(0000);
    int fd = open((char *)pMF->file_name, O_CREAT | O_RDWR, 0777);
    if (fd < 0)
    {
        perror("Creating downloaded file");
        exit(1);
    }
    Msg *pRequest = malloc(sizeof(Msg));
    Msg *pPiece = malloc(sizeof(Msg));
    int num_pieces = fileNumPieces(pMF);
    int num_blocks, offset, piece_size, break_while = 0;
    uint32_t piece;
    while ((break_while == 0) && ((piece = getClientPiece(pFS, peer_piece_map)) != -1)) // peer has the piece
    {
        printFileState(pFS);
        if (piece == num_pieces - 1)
            piece_size = (pMF->file_size % pMF->piece_size) == 0 ? pMF->piece_size : pMF->file_size % pMF->piece_size;
        else
            piece_size = pMF->piece_size;
        num_blocks = piece_size / FIXED_BLOCK_SIZE + (piece_size % FIXED_BLOCK_SIZE != 0);
        for (int block_offset = 0; block_offset < num_blocks; block_offset++)
        {
            // printf("Requesting piece %d - BLock %d\n", piece, block_offset);
            bzero(pRequest, sizeof(Msg));
            setPeerRequest(pRequest, piece, block_offset);
            if (sendMsg(peer_sockfd, pRequest) <= 0)
            {
                clearClientPiece(pFS, piece);
                break_while = 1;
                break;
            }
            bzero(pPiece, sizeof(Msg));
            if (recvMsg(peer_sockfd, pPiece) <= 0)
            {
                clearClientPiece(pFS, piece);
                break_while = 1;
                break;
            }
            if ((pPiece->payload.peer_message.message_id != PIECE) || (pPiece->payload.peer_message.payload.piece.piece_index < 0))
            {
                clearClientPiece(pFS, piece);
                break;
            }
            // printf("Size = %d\n", ntohl(pPiece->payload.peer_message.message_length) - 9);
            offset = piece * pMF->piece_size + block_offset * FIXED_BLOCK_SIZE;
            if (lseek(fd, offset, SEEK_SET) < 0)
            {
                perror("seek write file");
                exit(1);
            }
            write(fd, pPiece->payload.peer_message.payload.piece.data, ntohl(pPiece->payload.peer_message.message_length) - 9);
        }
        if (checkPieceHash(pMF, piece) != 0)
        {
            clearClientPiece(pFS, piece);
        }
    }
    close(fd);
    free(pRequest);
    free(pPiece);
    free(pCI);
    close(peer_sockfd);
    umask(mask);
    return;
}
/* downloads file specified in pFS, creates multiple threads to contatc all peers and request for pieces */
void downloadFile(File_State *pFS)
{
    // thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    // thread id
    pthread_t thr_id[MAX_NUM_PEERS];
    int num_pieces = fileNumPieces(pFS->file), num_peers;
    int errnum, thr_idx;
    while (1)
    {
        printFileState(pFS);
        if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
        {
            printf("mutex_lock file access: %s", strerror(errnum));
            exit(1);
        }
        printf("Waiting for new pieces\n");
        while (swarmInterestingPieces(pFS) == 0)
            pthread_cond_wait(&pFS->new_peers, &pFS->access);
        printf("Found interesting pieces\n");
        num_peers = pFS->num_peers;
        thr_idx = 0;
        for (int i = 0; i < num_peers; i++)
        {
            if (memcmp(pFS->peers[i].peer_id, pFS->client_info->peer_id, PEER_ID_BYTES) == 0)
                continue;
            Contact_Info *pCI = malloc(sizeof(pCI));
            contactInfoInit(pCI, pFS, i);
            if (pthread_create(&thr_id[thr_idx], &attr, (void *)contactPeer, (void *)pCI) != 0)
            {
                perror("pthread_create:");
                exit(1);
            }
            thr_idx++;
        }
        if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
        {
            printf("mutex_unlock file access: %s", strerror(errnum));
            exit(1);
        }
        // printf("num_peers = %d\n", num_peers);
        for (int i = 0; i < num_peers - 1; i++)
            pthread_join(thr_id[i], NULL);
        for (int i = 0; i < num_pieces; i++)
        {
            if ((checkPiece(pFS->client_piece_map, i) != 0) && (checkPieceHash(pFS->file, i) != 0))
                clearClientPiece(pFS, i);
        }
        if (checkPieceMap(pFS) == 1)
            break;
    }
}
/* prints file downloaded percentage*/
void printFileState(File_State *pFS)
{
    int num_pieces = fileNumPieces(pFS->file);
    int downloaded = 0, errnum, piece_size;
    Meta_File *pMF = pFS->file;
    if ((errnum = pthread_mutex_lock(&pFS->access)) != 0)
    {
        printf("mutex_lock file access: %s", strerror(errnum));
        exit(1);
    }
    for (int i = 0; i < num_pieces; i++)
    {
        if ((i == num_pieces - 1))
            piece_size = (pMF->file_size % pMF->piece_size) == 0 ? pMF->piece_size : pMF->file_size % pMF->piece_size;
        else
            piece_size = pMF->piece_size;
        downloaded += piece_size * ((checkPiece(pFS->client_piece_map, i) != 0) && (checkPieceHash(pMF, i) == 0));
    }
    double progress = (double)downloaded / (double)pMF->file_size;
    int barwidth = 50;
    int pos = progress * barwidth;
    printf("[");
    for (int i = 0; i < barwidth; i++)
    {
        if (i < pos)
            printf("=");
        else if (i == pos)
            printf(">");
        else
            printf(" ");
    }
    printf("] %d%%\n", (int)(progress * 100));

    if ((errnum = pthread_mutex_unlock(&pFS->access)) != 0)
    {
        printf("mutex_unlock file access: %s", strerror(errnum));
        exit(1);
    }
}