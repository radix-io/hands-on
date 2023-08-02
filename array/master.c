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

    /* HANDS-ON: gather all the array rows from every process to rank 0 */

    /* HANDS-ON: have rank zero do the I/O.  Will look a lot like hands-on #2,
     * except more data because rank 0 does all the work */


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
