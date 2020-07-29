#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "array.h"

int write_data(char *filename)
{
    /* We will store whatever additional information we want to retain about
     * the data in this struct */
    science data = {
        .row = YDIM,
        .col = XDIM,
        .iter = 1
    };

    int *array;
    int fd;
    int ret=0;

    /* defined in util.c, this routine will give us an initialized region of memory */
    array = buffer_create(0, XDIM, YDIM);

    fd = open(filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
    if (fd < 0) goto fn_error;

    /* we have two  memory regions -- the data and the struct containing
     * information about the data -- so we have to issue two write calls */
    ret = write(fd, &data, sizeof(data));
    if (ret < 0) goto fn_error;

    ret = write(fd, array, XDIM*YDIM*sizeof(int));
    if (ret < 0) goto fn_error;

    ret = close(fd);
    if (ret < 0) goto fn_error;

    free(array);
    return ret;

fn_error:
    perror("Error:");
    return ret;
}

int main(int argc, char **argv)
{
    int ret;

    ret = write_data("testfile");
}
