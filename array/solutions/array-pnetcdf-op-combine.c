#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "array.h"
#include "util.h"
#include <string.h>
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
    int varid_other;
    int iterations;
    MPI_Offset start[NDIMS], count[NDIMS];
    int reqs[2];
    int status[2];

    MPI_Info_create(&info);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    NC_CHECK(ncmpi_create(comm, filename, NC_CLOBBER, info, &ncfile));

    /* row-major ordering */
    NC_CHECK(ncmpi_def_dim(ncfile, "rows", YDIM*nprocs, &(dims[0])) );
    NC_CHECK(ncmpi_def_dim(ncfile, "elements", XDIM, &(dims[1])) );

    NC_CHECK(ncmpi_def_var(ncfile, "array", NC_INT, NDIMS, dims,
                &varid_array));
    NC_CHECK(ncmpi_def_var(ncfile, "other array", NC_INT, NDIMS, dims,
		&varid_other));

    iterations=1;
    NC_CHECK(ncmpi_put_att_int(ncfile, varid_array,
                "iteration", NC_INT, 1, &iterations));

    NC_CHECK(ncmpi_enddef(ncfile));

    values = buffer_create(rank, XDIM, YDIM);


    start[0] = rank*YDIM; start[1] = 0;
    count[0] = YDIM; count[1] = XDIM;
    NC_CHECK(ncmpi_iput_vara_int(ncfile, varid_array, start, count,
                values, &(reqs[0]) ) );
    NC_CHECK(ncmpi_iput_vara_int(ncfile, varid_other, start, count,
                values, &(reqs[1]) ) );

    /* all the I/O actually happens here */
    NC_CHECK(ncmpi_wait_all(ncfile, 2, reqs, status));
    NC_CHECK(ncmpi_close(ncfile));

    MPI_Info_free(&info);
    buffer_destroy(values);

    return 0;

}

int read_data(MPI_Comm comm, char *filename)
{
    MPI_Info info;
    int ncfile;
    int *read_buf;
    int rank, nprocs;
    int i;
    /* assorted metadata about netcdf contents */
    MPI_Offset count[NDIMS], starts[NDIMS];
    char varname[NC_MAX_NAME+1];
    nc_type vartype;
    int nr_dims, nr_attrs;
    int dim_ids[NDIMS];
    MPI_Offset dim_lens[NDIMS];
    /* attribute on variable */
    int iterations;

    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

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
    /* While we can assume a convention of a 2d variable in this example,
     * "inq_var" still provides the dimension id array (dim_ids) as well as a
     * bunch of other information about the variable we can use to sanity-check
     * that the variable we are reading is what we expect */
    NC_CHECK(ncmpi_inq_var(ncfile, 0, varname, &vartype, &nr_dims, dim_ids,
                &nr_attrs));
    if (nr_dims != NDIMS || strncmp(varname, "array", strlen("array")) != 0)
        fprintf(stderr, "Error: unexpected variable %s of dim %d in file\n",
                varname, nr_dims);

    NC_CHECK(ncmpi_inq_dim(ncfile, dim_ids[0], NULL, &(dim_lens[0])) );
    NC_CHECK(ncmpi_inq_dim(ncfile, dim_ids[1], NULL, &(dim_lens[1])) );

    NC_CHECK(ncmpi_get_att_int(ncfile, 0,
                "iteration", &iterations));

    /* read a single column (count[1] = 1).
     * The column is nprocs tall (count[0] = nprocs
     * start reading from the first row (starts[0] = 0
     * and pick the column in the middle (starts[1] = XDIM/2) */
    count[0] = nprocs; count[1] = 1;
    starts[0] = 0;     starts[1] = XDIM/2;
    NC_CHECK(ncmpi_get_vara_int_all(ncfile, 0, starts, count, read_buf));

    NC_CHECK(ncmpi_close(ncfile));
    MPI_Info_free(&info);

    if (!rank) {
        printf("variable %s of shape %lld by %lld; experiment on iteration %d\n",
                varname, dim_lens[0], dim_lens[1], iterations);

        for (i=0; i<nprocs; i++)
            printf("%d ", read_buf[i]);
	printf("\n");
    }

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
    return ret;
}
