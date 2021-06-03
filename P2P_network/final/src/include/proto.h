/* simplified copy of bittorrent protocol as read from 
http://jonas.nitro.dk/bittorrent/bittorrent-rfc.html#anchor1 */

#ifndef PROTO_H_
#define PROTO_H_ 1

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#include "meta_file.h"

#define TRACKER_IP "127.0.0.1"
#define CLIENT_IP "127.0.0.1"
#define TRACKER_PORT 6881

#define PEER_ID_BYTES 20
#define MAX_NUM_PEERS 10 //less than 256
#define PIECE_SIZE 4096
#define FIXED_BLOCK_SIZE 1024                                  //less than PIECE_SIZE
#define PIECE_MAP_STRUCT_SIZE 8                                // NEVER CHANGE
#define PIECE_MAP_BYTES MAX_NUM_PIECES / PIECE_MAP_STRUCT_SIZE // MAX_NUM_PIECES should be multiple of 8

typedef enum
{
    TRACKER_REQUEST,  // request a list of peers with file
    TRACKER_RESPONSE, //send a list of peers with file
    PEER_HANDSHAKE,   // peers presenting
    PEER_MESSAGE,     // peers requests and data transmission
} Type;

typedef struct __attribute__((__packed__))
{
    uint8_t type;          //type of msg
    uint32_t payload_size; //msg length, only payload, no header
} Header;

inline static uint16_t getPayloadSize(const Header *hdr)
{
    return ntohl(hdr->payload_size);
}

typedef struct __attribute__((packed)) //debeira agregar mas datos para que el server y los clientes elijan?
{
    uint8_t peer_id[PEER_ID_BYTES]; // peer unique id
    uint32_t peer_ip;               //IPv4
    uint16_t peer_port;             //port
} Peer_info;

typedef struct __attribute__((__packed__))
{
    uint8_t file_hash[SHA_DIGEST_LENGTH]; // SHA1sum of requested file
    Peer_info peer;                       //contact information of peer
    uint8_t piece_map[PIECE_MAP_BYTES];   //client piece map (pieces client has)
} Tracker_Request;

typedef struct __attribute__((__packed__))
{
    uint16_t interval;                  // comunication interval to tracker
    uint8_t num_peers;                  // number of peers with the file
    uint8_t piece_map[PIECE_MAP_BYTES]; // swarm (all peers) piece map
    Peer_info peers[MAX_NUM_PEERS];     // list of (num_peers) peers with the file
} Tracker_Response;

typedef struct __attribute__((__packed__))
{
    uint8_t file_hash[SHA_DIGEST_LENGTH]; // SHA1sum hash of file of interest
    uint8_t peer_id[PEER_ID_BYTES];       // peer id
    uint8_t piece_map[PIECE_MAP_BYTES];   // piece map of peer
} Peer_Handshake;

typedef enum
{
    REQUEST, // request a piece
    PIECE,   // send a piece
} Peer_msg_type;

typedef struct __attribute__((__packed__))
{
    uint32_t piece_index;  // file piece index
    uint32_t block_offset; // block offset within piece
} Peer_Request;

typedef struct __attribute__((__packed__))
{
    uint32_t piece_index;           // file piece index
    uint32_t block_offset;          // block offset within piece
    uint8_t data[FIXED_BLOCK_SIZE]; // block data
} Peer_Piece;

typedef struct __attribute__((__packed__))
{
    uint32_t message_length; // length of message excluding message length
    uint8_t message_id;      // id of peer_mesage (request or piece)
    union __attribute__((__packed__))
    {
        Peer_Request request;
        Peer_Piece piece;
    } payload; // peer_message to send

} Peer_Message;

typedef struct __attribute__((__packed__))
{
    Header hdr; // header of message
    union __attribute__((__packed__))
    {
        Tracker_Request tracker_request;
        Tracker_Response tracker_response;
        Peer_Handshake peer_handshake;
        Peer_Message peer_message;
    } payload; // payload
} Msg;

inline static void setPeerInfo(Peer_info *peer_info, uint8_t peer_id[PEER_ID_BYTES], uint32_t peer_ip, uint16_t peer_port)
{
    memcpy(peer_info->peer_id, peer_id, PEER_ID_BYTES);
    peer_info->peer_ip = htonl(peer_ip);
    peer_info->peer_port = htons(peer_port);
}

inline static void setTackerRequest(Msg *msg, uint8_t file_hash[SHA_DIGEST_LENGTH], Peer_info peer, uint8_t piece_map[PIECE_MAP_BYTES])
{
    msg->hdr.type = TRACKER_REQUEST;
    msg->hdr.payload_size = htonl(sizeof(Tracker_Request));
    memcpy(msg->payload.tracker_request.file_hash, file_hash, SHA_DIGEST_LENGTH);
    msg->payload.tracker_request.peer = peer;
    memcpy(msg->payload.tracker_request.piece_map, piece_map, PIECE_MAP_BYTES);
}

inline static void setTackerResponse(Msg *msg, uint16_t interval, uint8_t num_peers, uint8_t piece_map[PIECE_MAP_BYTES], Peer_info peers[MAX_NUM_PEERS])
{
    msg->hdr.type = TRACKER_RESPONSE;
    int response_size = sizeof(Tracker_Response) - ((MAX_NUM_PEERS - num_peers) * sizeof(Peer_info)); //size acording to number of peers sent
    msg->hdr.payload_size = htonl(response_size);
    msg->payload.tracker_response.interval = htons(interval);
    msg->payload.tracker_response.num_peers = num_peers;
    memcpy(msg->payload.tracker_response.piece_map, piece_map, PIECE_MAP_BYTES);
    memcpy(msg->payload.tracker_response.peers, peers, num_peers * sizeof(Peer_info)); // only copy number of peers sent
}

inline static void setPeerHandshake(Msg *msg, uint8_t file_hash[SHA_DIGEST_LENGTH], uint8_t peer_id[PEER_ID_BYTES], uint8_t piece_map[PIECE_MAP_BYTES])
{
    msg->hdr.type = PEER_HANDSHAKE;
    msg->hdr.payload_size = htonl(sizeof(Peer_Handshake));
    memcpy(msg->payload.peer_handshake.file_hash, file_hash, SHA_DIGEST_LENGTH);
    memcpy(msg->payload.peer_handshake.peer_id, peer_id, PEER_ID_BYTES);
    memcpy(msg->payload.peer_handshake.piece_map, piece_map, PIECE_MAP_BYTES);
}

inline static void setPeerRequest(Msg *msg, uint32_t piece_index, uint32_t block_offset)
{
    msg->hdr.type = PEER_MESSAGE;
    int payload_size = sizeof(Peer_Message) - sizeof(Peer_Piece) + sizeof(Peer_Request); //size of peer_request (for enum in Peer_Message)
    msg->hdr.payload_size = htonl(payload_size);
    msg->payload.peer_message.message_length = htonl(1 + sizeof(Peer_Request));
    msg->payload.peer_message.message_id = REQUEST;
    msg->payload.peer_message.payload.request.piece_index = htonl(piece_index);
    msg->payload.peer_message.payload.request.block_offset = htonl(block_offset);
}

inline static void setPeerPiece(Msg *msg, uint32_t data_size, uint32_t piece_index, uint32_t block_offset, uint8_t data[FIXED_BLOCK_SIZE])
{
    msg->hdr.type = PEER_MESSAGE;
    int payload_size = sizeof(Peer_Message) - FIXED_BLOCK_SIZE + data_size; // for the last block of a piece
    msg->hdr.payload_size = htonl(payload_size);
    int message_length = 1 + sizeof(Peer_Piece) - FIXED_BLOCK_SIZE + data_size;
    msg->payload.peer_message.message_length = htonl(message_length);
    msg->payload.peer_message.message_id = PIECE;
    msg->payload.peer_message.payload.piece.piece_index = htonl(piece_index);
    msg->payload.peer_message.payload.piece.block_offset = htonl(block_offset);
    memcpy(msg->payload.peer_message.payload.piece.data, data, data_size);
}

int sendMsg(int sockfd, const Msg *msg);
int recvMsg(int sockfd, Msg *msg);

#endif // PROTO_H_
