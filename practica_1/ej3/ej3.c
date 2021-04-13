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

void _ls(const char *dir)
{
    struct dirent *dp;
    struct stat s;
    struct passwd *pwd;
    struct group *grp;
    struct tm *tm;
    char datestring[256];
    /*Abro el directorio*/
    DIR *dh = opendir(dir);
    if (!dh)
    {
        perror("Opening directory");
        exit(1);
    }
    /* Me mudo a ese directorio, para no tener problemas con el stat*/
    if (chdir(dir) < 0)
    {
        perror("Changing dir");
        exit(1);
    }

    /* mientras no llegue a un puntero dirent nulo sigo*/
    while ((dp = readdir(dh)) != NULL)
    {
        /* evito las carpetas ocultas*/
        if (dp->d_name[0] == '.')
        {
            continue;
        }

        else
        {
            /* obtengo la stat y la guardo en s*/
            if (stat(dp->d_name, &s) == -1)
            {
                perror("Stat");
                exit(1);
            }
            /* Lo de abajo lo copie de https://pubs.opengroup.org/onlinepubs/009696699/functions/stat.html */
            /*print type, permissions and number of links*/
            printf("(%3o)", 0777 & s.st_mode);
            printf("%4d", s.st_nlink);
            /* Print out owner's name if it is found using getpwuid(). */
            if ((pwd = getpwuid(s.st_uid)) != NULL)
                printf(" %-8.8s", pwd->pw_name);
            else
                printf(" %-8d", s.st_uid);
            /* Print out group name if it is found using getgrgid(). */
            if ((grp = getgrgid(s.st_gid)) != NULL)
                printf(" %-8.8s", grp->gr_name);
            else
                printf(" %-8d", s.st_gid);
            /* Print size of file. */
            printf(" %9jd", (intmax_t)s.st_size);
            tm = localtime(&s.st_mtime);
            /* Get localized date string. */
            strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
            printf(" %s %s\n", datestring, dp->d_name);
            if (S_ISDIR(s.st_mode))
            {
                /* Aca pongo esto para separar cuando se entra a un directorio*/
                printf("=============== %s ===============\n", dp->d_name);
                _ls(dp->d_name);
                printf("===================================\n");
                if (chdir("..") < 0)
                {
                    perror("Changing dir");
                    exit(1);
                }
            }
        }
    }
}

int main(int argc, const char *argv[])
{
    /* Si no le paso ningun argumento, busca en el directorio actual*/
    if (argc == 1)
    {
        _ls(".");
    }
    /* Si no, busca en el directorio que le dije*/
    else
    {
        _ls(argv[1]);
    }
    return 0;
}
