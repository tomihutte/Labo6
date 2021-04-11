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
    int vals[4] = {32, 512, 8192, 1048576};
    char *fnames[4] = {"F_32", "F_512", "F_8K", "F_1M"};
    int fd;
    int n_blocks = 10;
    struct stat st;
    char *filename;
    int hole_size;
    int data = 1;

    for (int i = 0; i < 4; i++)
    {
        filename = fnames[i];
        hole_size = vals[i];
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
        if (fd < 0)
        {
            perror("Opening file");
            exit(1);
        }
        if (write(fd, &data, sizeof(data)) < 0)
        {
            perror("Data write");
            exit(1);
        }
        for (int j = 0; j < 9; j++)
        {
            if (lseek64(fd, hole_size, SEEK_CUR) < 0)
            {
                perror("lseek64");
            }
            if (write(fd, &data, sizeof(data)) < 0)
            {
                perror("Data write");
                exit(1);
            }
        }
        printf("%s\n", filename);
        printf("Usefull informatio: %d bytes \n", 10 * sizeof(data));
        if (fstat(fd, &st) < 0)
        {
            perror("Stat");
            exit(1);
        }
        printf("Size: %9jd\n", (intmax_t)st.st_size);
        printf("Blocks: %9jd\n", (intmax_t)st.st_blocks);
        if (close(fd) < 0)
        {
            perror("Close");
        }
    }
    return 0;
}
