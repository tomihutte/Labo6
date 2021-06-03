#include "meta_file.h"
#include <libgen.h>
#define BUFF_SIZE 32768

//slices an array from arg begin to arg end
void *array_slice(void *array, int begin, int end)
{
    int data_size = sizeof(array[0]);
    void *sliced = malloc(data_size * (end - begin));
    memcpy(sliced, array + begin * data_size, (end - begin) * data_size);
    return sliced;
}
// fills a metafile pMFS structure with the info of the file specified by path
// and with piece size of piece_size
void makeMetaFile(Meta_File *pMF, char *path, uint32_t piece_size)
{
    struct stat st;
    if (stat(path, &st) == -1)
    {
        perror("makeMetaFile stat:");
        exit(1);
    }
    int num_pieces = st.st_size / piece_size + (st.st_size % piece_size != 0);
    unsigned char hash[SHA_DIGEST_LENGTH];
    char *filename = basename(strdup(path));
    memcpy(pMF->file_name, filename, strlen(filename));
    calc_sha1(path, hash, 0, st.st_size);
    memcpy(pMF->file_hash, hash, SHA_DIGEST_LENGTH);
    pMF->file_size = st.st_size;
    pMF->piece_size = piece_size;
    if (num_pieces == 1)
    {
        memcpy(pMF->pieces_hash, hash, SHA_DIGEST_LENGTH);
        return;
    }
    else
    {
        for (int i = 0; i < num_pieces; i++)
        {
            calc_sha1(path, hash, i * piece_size, piece_size);
            memcpy(pMF->pieces_hash[i], hash, SHA_DIGEST_LENGTH);
        }
        return;
    }
}
// writes the metafile struct pMF to a file named fname
void writeMetaFile(Meta_File *pMF, char *fname)
{
    mode_t mask;
    mask = umask(0000);
    int fd = open(fname, O_CREAT | O_WRONLY, 0777);
    umask(mask);
    if (fd < 0)
    {
        printf("File_name = %s\n", fname);
        perror("opening file to write meta info:");
        exit(1);
    }
    if (ftruncate(fd, 0) < 0)
    {
        perror("truncating file: ");
        exit(1);
    }
    if (write(fd, pMF, sizeof(Meta_File)) < 0)
    {
        perror("writing meta file:");
        exit(1);
    }
}
// loads the info from file fname to metafile pMF
void loadMetaFile(Meta_File *pMF, char *fname)
{
    int fd = open(fname, O_RDONLY);
    if (fd < 0)
    {
        perror("opening file to read meta info:");
        exit(1);
    }

    if (read(fd, pMF, sizeof(Meta_File)) < 0)
    {
        perror("reading from meta file:");
        exit(1);
    }
}
// prints all metafile pMF info
void printMetaFile(Meta_File *pMF)
{
    printf("File: %s\n", (char *)pMF->file_name);
    char hash_readable[SHA_DIGEST_LENGTH * 2];
    format_char_hexa(pMF->file_hash, hash_readable, SHA_DIGEST_LENGTH);
    printf("Hash: %s\n", hash_readable);
    printf("Size: %d bytes\n", pMF->file_size);
    printf("Pieces size: %d bytes\n", pMF->piece_size);
    int num_pieces = pMF->file_size / pMF->piece_size + (pMF->file_size % pMF->piece_size != 0);

    for (int i = 0; i < num_pieces; i++)
    {
        uint8_t *piece_hash = pMF->pieces_hash[i];
        format_char_hexa(piece_hash, hash_readable, SHA_DIGEST_LENGTH);
        printf("Piece %d hash: %s\n", i, hash_readable);
    }
}
// formats a char array into a hex readable way
void format_char_hexa(uint8_t *from, char *to, int N)
{
    for (int i = 0; i < N; i++)
    {
        sprintf((char *)&to[i * 2], "%02x", from[i]);
    }
}
/* returns the number of pieces in a file according to file size and piece size */
int fileNumPieces(Meta_File *pMF)
{
    int ret = pMF->file_size / pMF->piece_size + (pMF->file_size % pMF->piece_size != 0);
    return ret;
}
// returns 0 if file_hashes are the same or different from 0 otherwise
int compareFilesHash(uint8_t *hash1, uint8_t *hash2)
{
    return memcmp(hash1, hash2, SHA_DIGEST_LENGTH);
}
/* returns 0 if hashes are the same, different from zero otherwise */
int checkPieceHash(Meta_File *file, int piece_index)
{
    char *fname = (char *)file->file_name;
    unsigned char downloaded_hash[SHA_DIGEST_LENGTH];
    calc_sha1(fname, downloaded_hash, piece_index * file->piece_size, file->piece_size);
    return compareFilesHash(downloaded_hash, file->pieces_hash[piece_index]);
}
//calculates sha1 of numBytes bytes starting from offset
void calc_sha1(char *path, unsigned char hash[SHA_DIGEST_LENGTH], long int offset, const int numBytes)
{
    int file = open(path, O_RDONLY);
    if (file < 0)
    {
        perror("opening file to hash");
        exit(1);
    }
    if (lseek(file, offset, SEEK_SET) < 0)
    {
        perror("lseeking file to hash");
        exit(1);
    }

    SHA_CTX sha1;
    SHA1_Init(&sha1);
    char buffer[BUFF_SIZE];
    int bytesRead = 0, totalBytes = 0;
    while ((bytesRead = read(file, buffer, BUFF_SIZE)) > 0)
    {
        totalBytes += bytesRead;
        if (totalBytes >= numBytes)
        {
            SHA1_Update(&sha1, buffer, bytesRead - totalBytes + numBytes);
            break;
        }
        else
            SHA1_Update(&sha1, buffer, bytesRead);
    }
    if (bytesRead < 0)
    {
        perror("calc_sha1 from file");
        exit(1);
    }
    close(file);
    SHA1_Final(hash, &sha1);
}
