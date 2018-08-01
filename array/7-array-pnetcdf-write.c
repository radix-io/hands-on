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

    /* row-major ordering */
    NC_CHECK(ncmpi_def_dim(ncfile, "rows", YDIM*nprocs, &(dims[0])) );
    NC_CHECK(ncmpi_def_dim(ncfile, "elements", XDIM, &(dims[1])) );

    NC_CHECK(ncmpi_def_var(ncfile, "array", NC_INT, NDIMS, dims,
                &varid_array));

    iterations=1;
    NC_CHECK(ncmpi_put_att_int(ncfile, varid_array,
                "iteration", NC_INT, 1, &iterations));

    NC_CHECK(ncmpi_enddef(ncfile));

    values = buffer_create(rank, XDIM, YDIM);


    start[0] = rank*YDIM; start[1] = 0;
    count[0] = YDIM; count[1] = XDIM;
    NC_CHECK(ncmpi_put_vara_int_all(ncfile, varid_array, start, count,
                values) );

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
