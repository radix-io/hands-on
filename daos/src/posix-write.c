#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <mpi.h>

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
    int rank;
    int ranks;
    int iterations=ITER;
    size_t totalsz;
    off_t off;;
    double stime,etime,ttime,maxtime;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);

    /* defined in util.c, this routine will give us an initialized region of memory */
    array = buffer_create(0, XDIM, YDIM);


    fd = open(filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
    if (fd < 0) goto fn_error;

    totalsz = (sizeof(data) + sizeof(int)*XDIM*YDIM) * iterations;

    /* get the offset for each rank */
    ret = MPI_Exscan(&totalsz, &off, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);
    if (ret < 0) goto fn_error;

    MPI_Barrier(MPI_COMM_WORLD);
    stime = MPI_Wtime();

    /* seek to offset of first op */
    if (rank > 0) off = lseek(fd, off, SEEK_SET);

    for (int i = 0; i < iterations; i++)
    {

    data.iter = i+1;

    /* we have two  memory regions -- the data and the struct containing
     * information about the data -- so we have to issue two write calls */
    ret = write(fd, &data, sizeof(data));
    if (ret < 0) goto fn_error;

    ret = write(fd, array, XDIM*YDIM*sizeof(int));
    if (ret < 0) goto fn_error;

    }

    ret = close(fd);
    if (ret < 0) goto fn_error;

    MPI_Barrier(MPI_COMM_WORLD);
    etime = MPI_Wtime();
    ttime = etime - stime;
    MPI_Reduce(&ttime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
       printf("total time; %lf second\n", maxtime);
       printf("data size: %" PRId64 " KiB\n", sizeof(int)*XDIM*YDIM/1024);
       printf("total size: %" PRId64 " KiB\n", totalsz * ranks / 1024);
       printf("BW: %lf MiB/s\n", (double)(totalsz * ranks) / 1024 / 1024 / maxtime);
       printf("IOps %lf Kops\n", (double)(iterations*2*ranks) / 1000 / maxtime);
    }

    buffer_destroy(array);
    return ret;

fn_error:
    perror("Error:");
    return ret;
}

int main(int argc, char **argv)
{
    int ret;

    ret = MPI_Init(&argc, &argv);

    ret = write_data("testfile");

    ret = MPI_Finalize(); 
}
