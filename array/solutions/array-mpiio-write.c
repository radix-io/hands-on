#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "mpi-util.h"

#define NDIMS 2
#define XDIM 5
#define YDIM 1

/* MPI-IO will move bytes for you but it won't do anything to describe the data
 * or give you any clue how it is structred.  This small struct is one way to
 * capture that structure: it describes the shape of the two-dimensional array
 * (row by col).  The third "iter" member describes which iteration of this
 * simulation is being captured.  You could imagine more sophisticated
 * descriptions: this is only a toy demonstration */
typedef struct {
    int row;
    int col;
    int iter;
} Science;

int write_data(MPI_Comm comm, char *filename)
{
    MPI_File fh;
    MPI_Info info;
    int *values;
    int rank, nprocs;
    Science header;


    MPI_Info_create(&info);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    /* try this both ways:  if "automatic" -- the default -- ROMIO will see
     * these requests are not "interleaved" and will just take the independent
     * contiguous path.
     *
     * You can turn off collective I/O and force independent I/O with "disable"
     */
    MPI_Info_set(info, "romio_cb_write", "enable");
    MPI_CHECK(MPI_File_open(comm, filename,
                MPI_MODE_CREATE|MPI_MODE_WRONLY, info, &fh));

    values = buffer_create(rank, XDIM, YDIM);
    header.row = nprocs*YDIM;
    header.col = XDIM;
    header.iter = 1;

    if (rank == 0) {
        MPI_CHECK(MPI_File_write(fh,
                    &header, sizeof(header), MPI_BYTE,
                    MPI_STATUS_IGNORE) );
    }
    MPI_CHECK(MPI_File_set_view(fh, sizeof(header),
                MPI_INT, MPI_INT, "native", info));
    MPI_CHECK(MPI_File_write_at_all(fh, rank*XDIM*YDIM,
            values, XDIM*YDIM, MPI_INT,
            MPI_STATUS_IGNORE));
    MPI_CHECK(MPI_File_close(&fh));

    MPI_Info_free(&info);

    return 0;
}

void read_data(MPI_Comm comm, char *filename)
{
    return;
}


int main(int argc, char **argv)
{
    int ret;
    int nprocs, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ret = write_data(MPI_COMM_WORLD, argv[1]);

    read_data(MPI_COMM_WORLD, argv[1]);

    MPI_Finalize();
    return ret;
}
