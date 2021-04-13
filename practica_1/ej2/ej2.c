#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

int main()
{
    int fd;

    umask(0700);

    fd = open("all_permitted", O_RDWR | O_CREAT, 0777);
    if (fd < 0)
    {
        perror("Open all_permited");
        exit(1);
    }
    if (close(fd) < 1)
    {
        perro("Close");
    }
    fd = open("no_ext", O_RDWR | O_CREAT, 0770);

    if (fd < 0)
    {
        perror("Open no_ext");
        exit(1);
    }

    if (close(fd) < 1)
    {
        perro("Close");
    }

    return 0;
}
