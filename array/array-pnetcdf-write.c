#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "array.h"
#include "util.h"
#include <pnetcdf.h>

#include "pnetcdf-util.h"

int write_data(MPI_Comm comm, char *filename)
{
    int ncfile;
    MPI_Info info;
    int *values;
    int rank, nprocs;
    int dims[NDIMS];
    int varid_array;
    int iterations;
    MPI_Offset start[NDIMS], count[NDIMS];

    MPI_Info_create(&info);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    NC_CHECK(ncmpi_create(comm, filename, NC_CLOBBER, info, &ncfile));

    /* HANDS-ON: define two dimensions.  Call one "rows" and the other
     * "elements" .  You will need to pass an array of identifiers when you
     * associate these dimensions with a variable: saves you a step if you pass
     * that array when you define dimension */

    /* HANDS-ON: define one variable, "array".  It has NDIMS dimensions. the
     * type is NC_INT */

    iterations=1;
    NC_CHECK(ncmpi_put_att_int(ncfile, varid_array,
                "iteration", NC_INT, 1, &iterations));

    /* Extra credit: what happens if you remove this line? */
    NC_CHECK(ncmpi_enddef(ncfile));

    values = buffer_create(rank, XDIM, YDIM);


    /* HANDS-ON: each process will write a subarray of the overall global
     * array.  call ncmpi_put_vara_int_all to write the process-local data to
     * the correct global location collectively */

    NC_CHECK(ncmpi_close(ncfile));

    MPI_Info_free(&info);
    buffer_destroy(values);

    return 0;

}

int read_data(MPI_Comm comm, char *filename)
{
        return 0;
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
