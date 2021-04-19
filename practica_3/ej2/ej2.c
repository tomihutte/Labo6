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

#define Q_SIZE 10

typedef struct
{
    sem_t sinc_acc;
    sem_t available_places;
    sem_t used_places;
    int elements[Q_SIZE];
    unsigned put;
    unsigned get;
} Queue_t;

// Inicializa (debe residir en un segmento de shared memory)
void QueueInit(Queue_t *pQ);
// Desstruye el contenedor, liberando recursos
void QueueDestroy(Queue_t *pQ);
// Agrega un Nuevo elemento. Bloquea si no hay espacio
void QueuePut(Queue_t *pQ, int elem);
// Remueve y retorna un elemento, bloquea si no hay elementos
int QueueGet(Queue_t *pQ);
// recupera la cantidad de elementos en la cola
int QueueSize(Queue_t *pQ);
// monitorea la cola
void QueueMonitor(Queue_t *pQ);

int main(int argc, const char *argv[])
{
    // hago el mapeo al vector pQ
    Queue_t *pQ = mmap(NULL, sizeof(Queue_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    //inicializo la la cola
    QueueInit(pQ);

    int pid, i;

    // forkeo un hijo
    pid = fork();
    switch (pid)
    {
    case -1:
        perror("fork child 1");
    case 0:
        //child - agrega elementos
        for (i = 0; i < 100; i++)
        {
            if ((i % 10) == 0)
            {
                //cada 10 quiero monitorear la cosa
                QueueMonitor(pQ);
            }
            // agrego un elemento
            QueuePut(pQ, i);
        }
        // el hijo termina aca
        return 0;
        break;
    default:
        //father - saca elementos
        for (i = 0; i < 100; i++)
        {
            QueueGet(pQ);
        }
        break;
    }
    // espero a que termine el hijo
    wait(NULL);
    printf("All processes finished\n");
    // monitoreo, deberia dar 0 elementos
    QueueMonitor(pQ);
    // vacio semaforos de la cola
    QueueDestroy(pQ);
    // destruyo el puntero a la cola compartido
    munmap(pQ, sizeof(Queue_t));
    return 0;
}

void QueueInit(Queue_t *pQ)
{

    // inicializo el semaforo de acceso
    if (sem_init(&pQ->sinc_acc, 1, 1) == -1)
        perror("sem_init acces");
    // inicializo el semaforo de lugares disponibles
    if (sem_init(&pQ->available_places, 1, Q_SIZE) == -1)
        perror("sem_init available_places");
    // inicializo el semaforo de lugares usados
    if (sem_init(&pQ->used_places, 1, 0) == -1)
        perror("sem_init used_places");
    // inicializo los indicadores de la cola
    pQ->put = 0;
    pQ->get = 0;
}

void QueueDestroy(Queue_t *pQ)
{
    // destruyo todos los semaforos
    if (sem_destroy(&pQ->sinc_acc) == -1)
        perror("sem_destroy sinc_acc");

    if (sem_destroy(&pQ->available_places) == -1)
        perror("sem_destroy available_places");

    if (sem_destroy(&pQ->used_places) == -1)
        perror("sem_destroy used_places");
}

void QueuePut(Queue_t *pQ, int elem)
{
    // espero a que haya lugar para agregar algo
    if (sem_wait(&pQ->available_places) == -1)
        perror("sem_wait available_places");
    // sincronizo el acceso
    if (sem_wait(&pQ->sinc_acc) == -1)
        perror("sem_wait sinc_acc");

    pQ->elements[pQ->put % Q_SIZE] = elem;
    pQ->put++;

    // libero el acceso
    if (sem_post(&pQ->sinc_acc) == -1)
        perror("sem_post sinc_acc");
    // aumento la cantidad de lugares ocupados
    if (sem_post(&pQ->used_places) == -1)
        perror("sem_post used_places");
}

int QueueGet(Queue_t *pQ)
{
    int ret;
    // espero a que haya algo para leer
    if (sem_wait(&pQ->used_places) == -1)
        perror("sem_wait used_places");
    // sincronizo el acceso
    if (sem_wait(&pQ->sinc_acc) == -1)
        perror("sem_wait sinc_acc");

    ret = pQ->elements[pQ->get % Q_SIZE];
    pQ->get++;

    // libero el acceso
    if (sem_post(&pQ->sinc_acc) == -1)
        perror("sem_post sinc_acc");
    // aumento la cantidad de lugares libres
    if (sem_post(&pQ->available_places) == -1)
        perror("sem_wait available_places");

    return ret;
}

int QueueSize(Queue_t *pQ)
{
    int ret;
    // sincronizo el acceso
    if (sem_wait(&pQ->sinc_acc) == -1)
        perror("sem_wait sinc_acc");

    ret = pQ->put - pQ->get;

    // libero el acceso
    if (sem_post(&pQ->sinc_acc) == -1)
        perror("sem_post sinc_acc");

    return ret;
}

void QueueMonitor(Queue_t *pQ)
{
    int i, n_elem = QueueSize(pQ);
    printf("Queue with %d elements\n", n_elem);
    for (i = 0; i < n_elem; i++)
    {
        printf("%d->", pQ->elements[(pQ->get + i) % Q_SIZE]);
    }
    printf("\n");
}