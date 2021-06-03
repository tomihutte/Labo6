#ifndef META_H_
#define META_H_ 1

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAX_FILE_NAME 128
#define MAX_NUM_PIECES 4096

typedef struct __attribute__((__packed__))
{
    uint8_t file_name[MAX_FILE_NAME];
    uint8_t file_hash[SHA_DIGEST_LENGTH];
    uint32_t file_size;
    uint32_t piece_size;
    uint8_t pieces_hash[MAX_NUM_PIECES][SHA_DIGEST_LENGTH];
} Meta_File;

void makeMetaFile(Meta_File *pMF, char *path, uint32_t piece_size);
void writeMetaFile(Meta_File *pMF, char *fname);
void loadMetaFile(Meta_File *pMF, char *fname);
void printMetaFile(Meta_File *pMF);
int fileNumPieces(Meta_File *pMF);
int compareFilesHash(uint8_t *hash1, uint8_t *hash2);
int checkPieceHash(Meta_File *file, int piece_index);
void calc_sha1(char *path, unsigned char hash[SHA_DIGEST_LENGTH], long int offset, const int numBytes);
void format_char_hexa(uint8_t *from, char *to, int N);

#endif // META_H_
