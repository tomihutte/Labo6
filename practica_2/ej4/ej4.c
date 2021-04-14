#define _GNU_SOURCE

#include <stdio.h>
#include <sys/time.h>
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

void father()
{
    /* para el select */
    fd_set read_set;
    struct timeval tv;
    /* datos que envio y recibo por el pipe */
    char send[8], receive[8];
    /* para escribir y no usar printf que anda raro */
    char no_data[] = "No data within five seconds\n";
    /* lectura de stdin, pipe, el select y si ya lei el stdin */
    int rd1, rd2, n_fd;
    int stdin_read = 1;
    int pipe_read = 1;

    /*cierro los extremos de los pipes que no voy a usar*/
    if (close(child_to_father[1]) < 0)
        perror("Father close child_to_father[1]:");

    if (close(father_to_child[0]) < 0)
        perror("Father close father_to_child[0]:");

    while (1)
    {
        FD_ZERO(&read_set);
        if (stdin_read)
            FD_SET(STDIN_FILENO, &read_set);
        FD_SET(child_to_father[0], &read_set);

        /* Wait up to five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        n_fd = select(child_to_father[0] + 1, &read_set, NULL, NULL, &tv);

        switch (n_fd)
        {
        case -1:
            perror("Father select: ");
            break;
        case 0:
            write(1, no_data, sizeof(no_data));
            break;
        default:
            if (FD_ISSET(STDIN_FILENO, &read_set))
            {
                /* leo de stdin */
                if ((rd1 = read(STDIN_FILENO, send, sizeof(send))) < 0)
                    perror("Father read from stdin");
                if (rd1 > 0)
                {
                    /* escribo al pipe father->child */
                    if (write(father_to_child[1], send, rd1) < 0)
                        perror("Father write to child");

                    /* cierro el pipe de father->child para que child ejecute*/
                    if (close(father_to_child[1]) < 0)
                        perror("Father close father_to_child[1]:");
                    /* para no leer de nuevo el stdin */
                    stdin_read = 0;
                }
                break;
            }
            if (FD_ISSET(child_to_father[0], &read_set))
            {
                /* leo del pipe child->father */
                if ((rd2 = read(child_to_father[0], receive, sizeof(receive))) < 0)
                    perror("Father read from child");
                if (rd2 > 0)
                {
                    /*cierro el pipe child-father*/
                    if (close(child_to_father[0]) < 0)
                        perror("Father close child_to_father[0]:");

                    /* escribo a stdout */
                    if (write(STDOUT_FILENO, receive, rd2) < 0)
                        perror("Father write to stdout");

                    pipe_read = 0;
                }
            }
        }
        if (!pipe_read)
        {
            break;
        }
    }
}

void child()
{
    /* cierro los extremos del pipe que no voy a usar */
    /*cierro los extremos de los pipes que no voy a usar*/
    if (close(child_to_father[0]) < 0)
        perror("Child close child_to_father[0]:");

    if (close(father_to_child[1]) < 0)
        perror("Child close father_to_child[1]:");

    /* hago que el STDIN vaya al mismo file que father_to_child[0] */
    if (dup2(father_to_child[0], STDIN_FILENO) < 0)
        perror("dup2 to stdin:");

    /* hago que el STDOUT vaya al mismo file que child_to_father[1] */
    if (dup2(child_to_father[1], STDOUT_FILENO) < 0)
        perror("dup2 to stdout:");

    /*ahora que tengo stdin y stdout puedo cerrar los extremos del pipe*/
    if (close(child_to_father[1]) < 0)
        perror("Child close child_to_father[1]:");
    if (close(father_to_child[0]) < 0)
        perror("Child close father_to_child[0]:");

    /* el programa usa getchar(), que espera de STDIN, que ahora es father_to_child[0] */
    if (execl("/home/tomas/labo6/Labo6/practica_2/ej3/filtro", "filtro", NULL) < 0)
        perror("Child excec:");
}

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    /* para leer constantemente los caracteres */
    while (1)
    {
        /* si hay un error con el pipe*/
        if (pipe(father_to_child) == -1)
        {
            perror("Pipe father->child");
            return -1;
        }

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
            /* ejecuto child */
            child();
            break;

        case -1:
            perror("Fork error:");
            break;

        default:
            /* ejecuto father  */
            father();
            /* para limpiar el zombie */
            wait(&status);
            /* para limpiar la stdin */
            fseek(stdin, 0, SEEK_END);
            break;
        }
        if (pid == 0)
            /* esto es para que el child no siga ejecutando el while y termine, asi el father
            puede seguir creando pipes y otros child sin problemas */
            break;
    }
    return 0;
}
