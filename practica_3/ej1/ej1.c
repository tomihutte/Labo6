#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#define n_signatures 2
#define signature_len 7
#define SH_BUF_SIZE 8

const char signature_1[signature_len] = "AAAAAA";
const char signature_2[signature_len] = "BBBBBB";
int child1, child2;

typedef struct
{
    sem_t lock;
    char buff[SH_BUF_SIZE];
} Data_t;

void initData(Data_t *pDat)
{
    sem_init(&pDat->lock, 1, 1);
    pDat->buff[0] = '\0';
}

void lockData(Data_t *pDat)
{
    sem_wait(&pDat->lock);
}

void unlockData(Data_t *pDat)
{
    sem_post(&pDat->lock);
}

void child(const char signature[], Data_t *pDat)
{
    int i, lb = 0;

    while (1)
    {
        // lockData(pDat);
        if (lb >= SH_BUF_SIZE - signature_len)
            break;
        for (i = 0; i < signature_len; i++)
        {
            pDat->buff[lb + i] = signature[i];
        }
        // unlockData(pDat);
    }
}

void father(Data_t *pDat)
{
    char n = '\n';
    int i, pos = 0;
    while (1)
    {
        lockData(pDat);
        for (i = pos; i < pos + signature_len; i++)
        {
            write(STDOUT_FILENO, &pDat->buff[i], sizeof(char));
        }

        write(STDOUT_FILENO, &n, sizeof(char));

        unlockData(pDat);

        if (pos >= SH_BUF_SIZE - signature_len)
            break;
    }
}

int main(int argc, char **argv)
{
    Data_t *p = mmap(NULL, sizeof(Data_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (p == MAP_FAILED)
    {
        perror("mmap: ");
        return 1;
    }

    initData(p);

    if ((child1 = fork()) == -1)
    {
        perror("Forking child 1:");
    }

    else if (child1 == 0)
    {
        child(signature_1, p);
    }

    else
    {
        if ((child2 = fork()) == -1)
        {
            perror("Forking child 2:");
        }
        else if (child2 == 0)
        {
            child(signature_2, p);
        }
        else
        {
            father(p);
        }
    }
    munmap(p, sizeof(Data_t));
    return 0;
}
