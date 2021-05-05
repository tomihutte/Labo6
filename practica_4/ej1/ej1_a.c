#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define NUM_WORKER_THREADS 8
#define MAX_QUEUE_SIZE 5
#define NUM_FUNC_CALLS 30
#define ARG_MOD 5

typedef void (*ProcFunc_t)(void *ctx);
typedef unsigned int WorkUnitId;

typedef struct
{
    // timeval porque me gusta mas
    struct timeval submitTime;    // submission time
    struct timeval startProcTime; // processing start time
    struct timeval endProcTime;   // processing end time
} WorkUnitStat_t;

typedef struct
{
    WorkUnitId id;
    ProcFunc_t fun;
    void *context;
    WorkUnitStat_t stats;
} WorkUnit_t;

typedef struct
{
    // es una cola circular
    WorkUnit_t units[MAX_QUEUE_SIZE];
    unsigned int get;
    unsigned int put;
    // sincroniza acceso
    pthread_mutex_t queue_access;
    // avisan que se puede sacar/poner cosas en la cola
    pthread_cond_t queue_get_ready;
    pthread_cond_t queue_put_ready;
} QueueWorkUnit_t;

typedef struct
{
    int n_units;
    double total_enqueued_seconds;
    double total_proc_seconds;
    // sincronizo el acceso para que ande todo bien
    pthread_mutex_t monitor_access;
} StatMonitor_t;

typedef struct
{
    // no entiendo bien por que el thr_id me da cualquier cosa
    // entonces uso el thr_num para reconocer el thread
    pthread_t thr_id;
    int thr_num;
    // tiene la cola y el stat monitor
    QueueWorkUnit_t *pQ;
    StatMonitor_t *pSM;
} WorkerThread_t;

typedef struct
{
    // cola - threads - stat monitor
    QueueWorkUnit_t *pQ;
    WorkerThread_t threads[NUM_WORKER_THREADS];
    StatMonitor_t *pSM;
} WorkServer_t;

typedef struct
{
    int num_calls;
    ProcFunc_t fake_func;
} FakeWorkUnitGen_t;

// no consumi recursos asiq ue no hay que liberar nada, no hace falta destroy
void WorkUnitStatInit(WorkUnitStat_t *pWUS)
{
    pWUS->submitTime.tv_sec = 0;
    pWUS->submitTime.tv_usec = 0;
    pWUS->startProcTime.tv_sec = 0;
    pWUS->startProcTime.tv_usec = 0;
    pWUS->endProcTime.tv_sec = 0;
    pWUS->endProcTime.tv_usec = 0;
}
// no consumi recursos asi que no hay que liberar nada, no hace falta destroy
void WorkUnitInit(WorkUnit_t *pWU, WorkUnitId id, void *ctx, ProcFunc_t func)
{
    pWU->id = id;
    pWU->context = ctx;
    pWU->fun = func;
    WorkUnitStatInit(&pWU->stats);
}

void QueueInit(QueueWorkUnit_t *pQ)
{
    //inicializo los indicadores
    pQ->get = 0;
    pQ->put = 0;
    int errnum;
    // inicializo mutex y las cond
    if ((errnum = pthread_mutex_init(&(pQ->queue_access), NULL)) != 0)
    {
        printf("mutex_init queu acces: %s\n", strerror(errnum));
    }
    if ((errnum = pthread_cond_init(&(pQ->queue_get_ready), NULL)) != 0)
    {
        printf("cond_init queu get: %s\n", strerror(errnum));
    }
    if ((errnum = pthread_cond_init(&(pQ->queue_put_ready), NULL)) != 0)
    {
        printf("cond_init queu put: %s\n", strerror(errnum));
    }
}

int QueueSize(QueueWorkUnit_t *pQ)
{
    return pQ->put - pQ->get;
}

WorkUnit_t QueueGet(QueueWorkUnit_t *pQ)
{
    WorkUnit_t WorkUnit;
    //bloqueo el mutex para poder acceder a la cola
    int errnum;
    if ((errnum = pthread_mutex_lock(&(pQ->queue_access))) != 0)
    {
        printf("mutex_lock q_acc q_get: %s\n", strerror(errnum));
        exit(1);
    }
    // mientras no haya nada en la cola espero para la condición
    while (QueueSize(pQ) == 0)
        pthread_cond_wait(&(pQ->queue_get_ready), &(pQ->queue_access));
    // si habia cosas en la cola o la condicion se cumplio
    WorkUnit = pQ->units[pQ->get % MAX_QUEUE_SIZE];
    pQ->get++;
    if ((errnum = pthread_cond_signal(&(pQ->queue_put_ready))) != 0)
    {
        printf("cond_signal put: %s\n", strerror(errnum));
        exit(1);
    }
    if ((errnum = pthread_mutex_unlock(&(pQ->queue_access))) != 0)
    {
        printf("mutex_unlock q_acc q_get: %s\n", strerror(errnum));
        exit(1);
    }
    return WorkUnit;
}

void QueuePut(QueueWorkUnit_t *pQ, WorkUnit_t *WorkUnit)
{
    //bloqueo el mutex para poder acceder a la cola
    int errnum;
    if ((errnum = pthread_mutex_lock(&(pQ->queue_access))) != 0)
    {
        printf("mutex_lock q_acc q_put: %s\n", strerror(errnum));
    }
    // si la cola esta llena, espero a que se libere, si no esta llena, no entra en el while
    while (QueueSize(pQ) == MAX_QUEUE_SIZE)
        pthread_cond_wait(&(pQ->queue_put_ready), &(pQ->queue_access));
    // una vez cumplida la condición
    pQ->units[pQ->put % MAX_QUEUE_SIZE] = *WorkUnit;
    pQ->put++;
    // aviso que se pueden agarrar cosas de la cola
    if ((errnum = pthread_cond_signal(&(pQ->queue_get_ready))) != 0)
    {
        printf("cond_signal get: %s\n", strerror(errnum));
        exit(1);
    }
    // desbloqueo el acceso a la cola
    if ((errnum = pthread_mutex_unlock(&(pQ->queue_access))) != 0)
    {
        printf("mutex_unlock q_acc q_put: %s\n", strerror(errnum));
        exit(1);
    }
}

void QueueDestroy(QueueWorkUnit_t *pQ)
{
    //libero mutex y cond
    int errnum;
    if ((errnum = pthread_mutex_destroy(&(pQ->queue_access))) != 0)
    {
        printf("mutex_destroy queu acces: %s\n", strerror(errnum));
    }
    if ((errnum = pthread_cond_destroy(&(pQ->queue_get_ready))) != 0)
    {
        printf("cond_destroy queu get: %s\n", strerror(errnum));
    }
    if ((errnum = pthread_cond_destroy(&(pQ->queue_put_ready))) != 0)
    {
        printf("cond_destroy queu put: %s\n", strerror(errnum));
    }
}

void StatMonitorInit(StatMonitor_t *pSM)
{
    // inicializo todo en cero y el mutex
    pSM->n_units = 0;
    pSM->total_enqueued_seconds = 0;
    pSM->total_proc_seconds = 0;
    int errnum;
    if ((errnum = pthread_mutex_init(&(pSM->monitor_access), NULL)) != 0)
    {
        printf("mutex_init monitor acces: %s\n", strerror(errnum));
    }
}
//agrega las estadisticas del workunitstat al stat monitor
void StatMonitorUpdate(StatMonitor_t *pSM, WorkUnitStat_t *pWUS)
{
    int errnum;
    if ((errnum = pthread_mutex_lock(&(pSM->monitor_access))) != 0)
    {
        printf("mutex_lock monitor_acc: %s\n", strerror(errnum));
    }
    // como le agregue una unidad
    pSM->n_units++;
    // paso los timeval a float
    double enqueued_time = (pWUS->startProcTime.tv_sec + pWUS->startProcTime.tv_usec / 1e6) - (pWUS->submitTime.tv_sec + pWUS->submitTime.tv_usec / 1e6);
    double process_time = (pWUS->endProcTime.tv_sec + pWUS->endProcTime.tv_usec / 1e6) - (pWUS->startProcTime.tv_sec + pWUS->startProcTime.tv_usec / 1e6);
    // el tiempo total
    pSM->total_enqueued_seconds += enqueued_time;
    pSM->total_proc_seconds += process_time;
    if ((errnum = pthread_mutex_unlock(&(pSM->monitor_access))) != 0)
    {
        printf("mutex_unlock monitor_acc: %s\n", strerror(errnum));
    }
}
// imprime estadisticas de la arquitecturs
void StatMonitorPrint(StatMonitor_t *pSM)
{
    printf("Total units executed: %d\n", pSM->n_units);
    printf("Average unit wait in queue: %f\n", pSM->total_enqueued_seconds / pSM->n_units);
    printf("Average execution time: %f\n", pSM->total_proc_seconds / pSM->n_units);
}

void StatMonitorDestroy(StatMonitor_t *pSM)
{
    int errnum;
    // solo tengo que destruir el mutex
    if ((errnum = pthread_mutex_destroy(&(pSM->monitor_access))) != 0)
    {
        printf("mutex_destroy monitor acces: %s\n", strerror(errnum));
    }
}

// monitorea la cola, esperando a que pueda sacar una WorkUnit para trabajar
void *thread_function(void *arg)
{
    // casteo el arg como el puntero al working thread
    WorkerThread_t *pWT = (WorkerThread_t *)arg;
    WorkUnit_t WorkUnit;
    while (1)
    {
        WorkUnit = QueueGet(pWT->pQ);
        // si le llega la NULL, andate bro
        if (WorkUnit.fun == NULL)
            break;
        // declaring work start
        printf("Thread %d working with unit %u\n", pWT->thr_num, WorkUnit.id);
        // set start proc time at workunit stat
        if (gettimeofday(&(WorkUnit.stats).startProcTime, NULL) == -1)
            perror("gettimeofday");
        WorkUnit.fun(WorkUnit.context);
        // set finish proc time at workunit stat
        if (gettimeofday(&(WorkUnit.stats).endProcTime, NULL) == -1)
            perror("gettimeofday");
        // declaring work finished
        printf("Thread %d finished working with unit %u\n", pWT->thr_num, WorkUnit.id);
        // agrego todo esto al stat monitor
        StatMonitorUpdate(pWT->pSM, &(WorkUnit.stats));
    }
    pthread_exit(NULL);
}

// el destroy de los threads lo hago desde el WorkServer
void WorkerThreadInit(WorkerThread_t *pWT, int thr_id, QueueWorkUnit_t *pQ, StatMonitor_t *pSM, pthread_attr_t attr)
{
    // uso thr_num porque thr_id anda medio raro, no me imprime el numero que quiero
    pWT->pQ = pQ;
    pWT->pSM = pSM;
    pWT->thr_num = thr_id;
    pWT->thr_id = thr_id;
    int errnum;
    printf("Initializing thread %d\n", thr_id);
    // creo el thread
    if ((errnum = pthread_create(&(pWT->thr_id), &attr, thread_function, (void *)pWT)) != 0)
    {
        printf("pthread_create: %s\n", strerror(errnum));
        exit(1);
    }
}

void WorkServerInit(WorkServer_t *pWS)
{
    printf("Initializing server with %d WorkingThreads\n", NUM_WORKER_THREADS);
    // alloco e inicializo la cola
    pWS->pQ = malloc(sizeof(QueueWorkUnit_t));
    QueueInit(pWS->pQ);
    // alloco e inicializo el stat monitor
    pWS->pSM = malloc(sizeof(StatMonitor_t));
    StatMonitorInit(pWS->pSM);
    // creo los attr para los threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int i;
    for (i = 0; i < NUM_WORKER_THREADS; i++)
    {
        // creo los threads donde el thr_id es i
        WorkerThreadInit(&(pWS->threads[i]), i, pWS->pQ, pWS->pSM, attr);
    }
}

// va a dormir por la cantidad de segundos que se le pasen como argumento
void fake_function(void *ctx)
{
    // aca simplemente duerme la cantidad de segundos que le pase como argumento
    int id = (int *)ctx;
    // printf("sleep(%d)\n", id);
    sleep(id);
}

void WorkServerDestroy(WorkServer_t *pWS)
{
    int i;
    // esto es para que todos los threads terminen
    WorkUnit_t *finishWU = malloc(sizeof(WorkUnit_t));
    finishWU->fun = NULL;
    for (i = 0; i < NUM_WORKER_THREADS; i++)
    {
        // mando igual que la cantidad de threads, van a terminar
        QueuePut(pWS->pQ, finishWU);
    }
    // joineo todos los threads
    for (i = 0; i < NUM_WORKER_THREADS; i++)
    {
        pthread_join(pWS->threads[i].thr_id, NULL);
        printf("Thread %d joined\n", pWS->threads[i].thr_num);
    }
    // imprimo los stat porque pinta
    StatMonitorPrint(pWS->pSM);
    // destruyo la queue y stat monitor
    QueueDestroy(pWS->pQ);
    free(pWS->pQ);
    StatMonitorDestroy(pWS->pSM);
    free(pWS->pSM);
    free(finishWU);
}

// mete la workunit en la cola del serber
void WorkServerSubmit(WorkServer_t *pWS, WorkUnit_t *WorkUnit)
{
    // antes de mandar algo a la cola, le pongo el tiempo de subida al stat monitor
    if (gettimeofday(&((WorkUnit->stats).submitTime), NULL) == -1)
        perror("gettimeofday\n");
    QueuePut(pWS->pQ, WorkUnit);
}

// manda NUM_FUNC_CALLS workunits a la cola
void FakeWorkUnitGenStart(FakeWorkUnitGen_t *pFWUG, WorkServer_t *pWS, int arg_mod)
{
    pFWUG->num_calls = NUM_FUNC_CALLS;
    pFWUG->fake_func = &fake_function;
    WorkUnit_t *WorkUnit = malloc(sizeof(WorkUnit_t));
    int i;
    for (i = 0; i < NUM_FUNC_CALLS; i++)
    {
        int d = i % arg_mod;
        WorkUnitInit(WorkUnit, i, d, pFWUG->fake_func);
        WorkServerSubmit(pWS, WorkUnit);
    }
    free(WorkUnit);
}

int main(int argc, char *argv[])
{
    int argmod = 5;
    WorkServer_t *pWS = malloc(sizeof(WorkServer_t));
    FakeWorkUnitGen_t *pFWUG = malloc(sizeof(FakeWorkUnitGen_t));
    WorkServerInit(pWS);
    FakeWorkUnitGenStart(pFWUG, pWS, argmod);
    WorkServerDestroy(pWS);
    free(pWS);
    free(pFWUG);
}
