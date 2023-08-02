#include <mpi.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <stdio.h>

#include "util.h"
#include "mpi-util.h"
#include "array.h"

int write_data(MPI_Comm comm, char *filename)
{
    int rank, nprocs;
    science data;

    int *array;
    int fd=0;
    int ret=0;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);
    /* every process creates its own buffer */
    array = buffer_create(rank, XDIM, YDIM);

    /* and then sends it to rank 0  */
    int *buffer = malloc(XDIM*YDIM*nprocs*sizeof(int));

    MPI_CHECK(MPI_Gather(/* sender (buffer,count,type) tuple */
            array, XDIM*YDIM, MPI_INT,
            /* receiver tuple */
            buffer, XDIM*YDIM, MPI_INT,
            /* who gathers and across which context */
            0, comm));

    if (rank == 0) {
        fd = open(filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
        if (fd < 0) goto fn_error;

        /* writing a global array, not just our local piece of it */
        data.row = YDIM*nprocs;
        data.col = XDIM;
        data.iter = 1;

        ret = write(fd, &data, sizeof(data));
        if (ret < 0) goto fn_error;

        ret = write(fd, buffer, XDIM*YDIM*nprocs*sizeof(int));
        if (ret < 0) goto fn_error;

        ret = close(fd);
        if (ret < 0) goto fn_error;
    }

    free(array);
    return ret;

fn_error:
    perror("Error:");
    if (fd != 0) close(fd);
    return ret;
}

int main(int argc, char **argv)
{
    int ret;
    MPI_Init(&argc, &argv);

    ret = write_data(MPI_COMM_WORLD, argv[1]);

    MPI_Finalize();
    return ret;
}
