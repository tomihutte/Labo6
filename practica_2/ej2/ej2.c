#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

int child_to_father[2];

void catch_int_info(int sig_num, siginfo_t *psiginfo, void *pcontext)
{
    /*manejo de la señal*/
    printf("(%d) Received signal #%d \"%s\" from %d, code: %d\n", getpid(), sig_num,
           strsignal(sig_num), psiginfo->si_pid, psiginfo->si_code);
}

void father()
{
    char receive[128];
    int rd;

    /*cierro los extremos de los pipes que no voy a usar*/
    if (close(child_to_father[1]) < 0)
        perror("Father close write pipe end:");

    /* voy a leer del child hasta que no haya mas bytes para leer
    o me de error */
    while ((rd = read(child_to_father[0], receive, sizeof(receive))) > 0)
    {
        /* si hay un problema con la escritura */
        if (write(STDOUT_FILENO, receive, rd) < 0)
            perror("Father write to stdout:");
    }
    /* si el read me dio problemas (sali del while ya) */
    if (rd < 0)
        perror("Father read from child:");

    /* cierro los otros extremos del pipe */
    if (close(child_to_father[0]) < 0)
        perror("Father close read pipe end:");
}

void child(int argc, char *argv[])
{
    /* voy a pasarle los argumentos del ejecutable */
    char *argv_child[argc + 1];
    /* el primero lo pongo como nombre del programa, para execv */
    argv_child[0] = "ls";
    for (int i = 1; i < argc; i++)
        argv_child[i] = argv[i];
    argv_child[argc] = NULL;

    /* cierro los extremos del pipe que no voy a usar */
    if (close(child_to_father[0]) < 0)
        perror("Closing read pipe from child: ");

    /* hago que el STOUT vaya al mismo file que child_to_father[1] */
    if (dup2(child_to_father[1], STDOUT_FILENO) < 0)
        perror("dup2 to stdout:");

    /* como ls imprime en el stout, ahora lo va a mandar al child_to_father[1] */
    if (execv("/usr/bin/ls", argv_child) < 0)
        perror("Child excecv:");

    /* cierro el extremo del pipe */
    close(child_to_father[1]);
}

int main(int argc, char *argv[])
{
    pid_t pid;
    struct sigaction act;

    /* Inicializo  el sigaction para el pipe*/
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = catch_int_info;
    sigfillset(&act.sa_mask);
    sigaction(SIGPIPE, &act, NULL);

    /* Inicializo  el sigaction para el pipe*/
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = catch_int_info;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);

    /* si hay un error con el pipe*/
    if (pipe(child_to_father) == -1)
    {
        perror("Pipe child->father");
        return -1;
    }
    pid = fork();
    switch (pid)
    {
    case 0:
        /* le paso los argumentos que vamos a usar */
        child(argc, argv);
        break;

    case -1:
        perror("Fork error:");
        break;

    default:
        father();
        break;
    }

    return 0;
}
