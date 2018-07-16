#include <mpi.h>
#include <pnetcdf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NDIMS 2
#define XDIM 5
#define YDIM 1

static void handle_error(int errcode, char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    //MPI_Abort(MPI_COMM_WORLD, 1);
}
#define MPI_CHECK(fn) { int errcode; errcode = (fn); if (errcode != MPI_SUCCESS) handle_error(errcode, #fn ); }

#define NC_CHECK(status) { int nc_status = status; if (nc_status != NC_NOERR) perror(ncmpi_strerror(nc_status)); }

int * buffer_create(int seed, int x, int y)
{
    int i;
    int *buffer = malloc(x*y*sizeof(int));
    for (i=0; i<x*y; i++)
    {
        buffer[i] = seed*10+i;
    }
    return buffer;

}

void buffer_destroy(int *buffer)
{
    free(buffer);
}

int write_data(MPI_Comm comm, char *filename)
{
    int ncfile;
    MPI_Info info;
    int *data;
    int rank, nprocs;
    int dims[NDIMS];
    int varid_array;
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

    NC_CHECK(ncmpi_enddef(ncfile));

    data = buffer_create(rank, XDIM, YDIM);


    start[0] = rank*YDIM; start[1] = 0;
    count[0] = YDIM; count[1] = XDIM;
    NC_CHECK(ncmpi_put_vara_int_all(ncfile, varid_array, start, count,
                data) );

    NC_CHECK(ncmpi_close(ncfile));

    MPI_Info_free(&info);
    buffer_destroy(data);

    return 0;
}

int read_data(MPI_Comm comm, char *filename)
{
    MPI_Info info;
    int ncfile;
    int *read_buf;
    int rank, nprocs;
    MPI_Offset count[NDIMS], starts[NDIMS];
    char varname[NC_MAX_NAME+1];
    nc_type vartype;
    int nr_dims, nr_attrs;
    int dims[NDIMS];
    int i;

    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    /* read a single collumn out of the array */
    read_buf = malloc(nprocs *sizeof(int));
    MPI_Info_create(&info);

    /* In C-order the arrays are row-major:
     *
     * |-----|
     * |-----|
     * |-----|
     *
     * The 'sizes' of the above array would be 3,5
     * The last collumn would be a "subsize" of 3,1
     * And a "start" of 0,5 */


    NC_CHECK(ncmpi_open(comm, filename, NC_NOWRITE, info, &ncfile));
    /* not necessary: could just read the first variable out of the file.
     * however, "inq_var" will let us sanity-check that the variable we are
     * reading is what we expect */
    NC_CHECK(ncmpi_inq_var(ncfile, 0, varname, &vartype, &nr_dims, dims,
                &nr_attrs));
    if (nr_dims != NDIMS || strncmp(varname, "array", strlen("array") != 0))
        fprintf(stderr, "Error: unexpected variable %s of dim %d in file\n",
                varname, nr_dims);

    count[0] = nprocs; count[1] = 1;
    starts[0] = 0;     starts[1] = XDIM/2;
    NC_CHECK(ncmpi_get_vara_int_all(ncfile, 0, starts, count, read_buf));

    NC_CHECK(ncmpi_close(ncfile));
    MPI_Info_free(&info);

    if (!rank) {
        for (i=0; i<nprocs; i++)
            printf("%d ", read_buf[i]);
    }
    printf("\n");

    free(read_buf);
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
}
