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

int father_to_child[2], child_to_father[2];

void catch_int_info(int sig_num, siginfo_t *psiginfo, void *pcontext)
{
    /*manejo de la señal*/
    printf("(%d) Received signal #%d \"%s\" from %d, code: %d\n", getpid(), sig_num,
           strsignal(sig_num), psiginfo->si_pid, psiginfo->si_code);
}

void father(int pid)
{
    char send[128];
    char receive[128];
    int rd1, rd2;

    /*cierro los extremos de los pipes que no voy a usar*/
    close(father_to_child[0]);
    close(child_to_father[1]);

    while (1)
    {
        /* leo de stdin */
        rd1 = read(STDIN_FILENO, send, sizeof(send));
        if (rd1 < 0)
        {
            /*uso break para que el programa continuo y 
            cierre las cosas si pasa algo*/
            perror("Read from stdin");
            break;
        }
        /* exribo al pipe father->child */
        if (write(father_to_child[1], send, rd1) < 0)
        {
            perror("Write father to child");
            break;
        }
        /* leo del pipe child->father */
        rd2 = read(child_to_father[0], receive, sizeof(receive));
        if (rd2 < 0)
        {
            perror("Read father from child");
            break;
        }
        /* escribo a stdout */
        if (write(STDOUT_FILENO, receive, rd2) < 0)
        {
            perror("Write father to stdout");
            break;
        }
    }
    /* cierro los otros extremos del pipe 
    cuando  hay un break */
    close(father_to_child[1]);
    close(child_to_father[0]);
}

void child()
{
    char receive[128], send[128];
    int rd;

    /* cierro los extremos del pipe que no voy a usar */
    close(child_to_father[0]);
    while (1)
    {
        /* leo del father->child */
        if ((rd = read(father_to_child[0], receive, sizeof(receive))) == -1)
        {
            perror("Child receive from father");
            /*uso break para que el programa continue y 
            cierre las cosas si pasa algo*/
            break;
        }
        /* convierto en mayuscula lo que lei */
        for (int i = 0; i < strlen(receive); i++)
        {
            send[i] = toupper(receive[i]);
        }
        /* envio al father lo que converti en mayusculas */
        if (write(child_to_father[1], send, rd) == -1)
        {
            perror("Child send to father");
            break;
        }
    }
    /* cierro los otros extremos del pipe 
    cuando  hay un break */
    close(father_to_child[0]);
    close(child_to_father[1]);
}

int main(int argc, char *argv[])
{
    pid_t pid;
    struct sigaction act;

    /* Inicializo  el sigaction para el child*/
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = catch_int_info;
    sigfillset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);

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
    if (pipe(father_to_child) == -1)
    {
        perror("Pipe father->child");
        return -1;
    }
    if (pipe(child_to_father) == -1)
    {
        perror("Pipe child->father");
        return -1;
    }
    /* Imprimo el PID del padre*/
    printf("[%s] Father pid is %d\n", argv[0], getpid());
    pid = fork();
    switch (pid)
    {
    case 0:
        /* Imprimo el PID del hijo*/
        printf("Child is %d\n", getpid());
        child();
        break;

    case -1:
        perror("error en el fork:");
        break;

    default:
        father(pid);
        break;
    }

    return 0;
}
