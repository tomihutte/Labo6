#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <langinfo.h>
#include <locale.h>

int main(int argc, char *argv[])
{
    int fd, hole_size;
    struct stat st;
    char *filename;
    /* le paso el filename y hole size por la linea de comandos*/
    if (argc > 3 || argc < 3)
    {
        printf("Error: Enter filename and hole size (ej4 filename hole_size");
        return 0;
    }

    filename = argv[1];
    hole_size = atoi(argv[2]);

    /* abro/creo un archivo*/

    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0)
    {
        perror("Opening file");
        exit(1);
    }

    /* escribo algo*/
    char *start = "Principio";

    if (write(fd, &start, sizeof(start)) < 0)
    {
        perror("Start write");
        exit(1);
    }

    /* corro el offset*/

    if (lseek64(fd, hole_size, SEEK_CUR) < 0)
    {
        perror("lseek64");
    }

    char *end = "Final";

    /* vuelvo e a escribir*/

    if (write(fd, &end, sizeof(end)) < 0)
    {
        perror("Start write");
        exit(1);
    }

    /* describo lo que hay en el stat*/

    printf("Usefull informatio: %d bytes \n", sizeof(end) + sizeof(start));

    if (fstat(fd, &st) == -1)
    {
        perror("Stat");
        exit(1);
    }

    printf("Datos de estructura stat: \n");
    /*print type, permissions and number of links*/
    printf("Permisos: (%3o)\n", 0777 & st.st_mode);
    printf("N links: %4d\n", st.st_nlink);
    /* Print size of file. */
    printf("Tamaño: %9jd\n", (intmax_t)st.st_size);
    return 0;

    if (close(fd) < 0)
    {
        perror("Close");
    }
}
