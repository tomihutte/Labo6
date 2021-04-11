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
    int fd;
    struct stat st;
    char *filename, soft_filename[20], hard_filename[20];

    if (argc > 2 || argc < 2)
    {
        printf("Error: Enter filename (ej6 filename) ");
        return 0;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0)
    {
        perror("Opening file");
        exit(1);
    }

    sprintf(soft_filename, "%s_soft", filename);
    sprintf(hard_filename, "%s_hard", filename);

    if (fstat(fd, &st) < 0)
    {
        perror("After creating stat");
        exit(1);
    }

    printf("Just created: %d links\n", st.st_nlink);

    if (symlink(filename, soft_filename) < 0)
    {
        perror("Soft link");
    }

    if (fstat(fd, &st) < 0)
    {
        perror("Stat sym");
        exit(1);
    }

    printf("After creating symbolic link: %d links\n", st.st_nlink);

    if (link(filename, hard_filename) < 0)
    {
        perror("Hard link");
    }

    if (fstat(fd, &st) < 0)
    {
        perror("Stat hard");
        exit(1);
    }

    printf("After creating hard link: %d links\n", st.st_nlink);

    if (unlink(soft_filename) < 0)
    {
        perror("Unlinking symlink");
    }

    if (fstat(fd, &st) < 0)
    {
        perror("Stat soft unlinked");
        exit(1);
    }

    printf("After unlinking soft link: %d links\n", st.st_nlink);

    if (unlink(filename) < 0)
    {
        perror("Unlinking hard link");
    }

    if (fstat(fd, &st) < 0)
    {
        perror("Stat file unlinked");
        exit(1);
    }

    printf("After unlinking file: %d links\n", st.st_nlink);

    if (unlink(hard_filename) < 0)
    {
        perror("Unlinking hard link");
    }

    if (fstat(fd, &st) < 0)
    {
        perror("Stat hard unlinked");
        exit(1);
    }

    printf("After unlinking hard link: %d links\n", st.st_nlink);

    return 0;
}
