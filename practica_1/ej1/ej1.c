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
    int i, fd, ret_wr, ret_cl;
    char *wr_errno = malloc(7 * sizeof(char));

    fd = open("try_open", O_RDWR | O_CREAT | O_EXCL, 0777);
    if (fd < 0)
    {
        perror("Open try_open");
        exit(1);
    }

    for (i = 0; i < 10; i++)
    {
        ret_wr = write(fd, &i, sizeof(i));
        if (ret_wr < 0)
        {
            sprintf(wr_errno, "%s %d", "Write", i);
            perror(wr_errno);
        }
    }

    ret_cl = close(fd);

    if (ret_cl < 0)
    {
        perror("Close");
    }

    return 0;
}
