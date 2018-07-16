#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

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
    MPI_File fh;
    MPI_Info info;
    int *data;
    int rank;

    MPI_Info_create(&info);

    MPI_Comm_rank(comm, &rank);

    MPI_CHECK(MPI_File_open(comm, filename, MPI_MODE_CREATE|MPI_MODE_WRONLY, info, &fh));

    data = buffer_create(rank, XDIM, YDIM);

    MPI_CHECK(MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, "native", info));
    MPI_CHECK(MPI_File_write_at_all(fh, rank*XDIM*YDIM, data, XDIM*YDIM,
                MPI_INT, MPI_STATUS_IGNORE));
    MPI_CHECK(MPI_File_close(&fh));

    MPI_Info_free(&info);

    return 0;
}

int read_data(MPI_Comm comm, char *filename)
{
    MPI_File fh;
    MPI_Info info;
    int *read_buf;
    int rank, nprocs;
    MPI_Datatype subarray;
    int sizes[NDIMS], sub[NDIMS], starts[NDIMS];
    int i;

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

    sizes[0] = nprocs;
    sizes[1] = XDIM;
    sub[0] = nprocs;
    sub[1] = 1;
    starts[0] = 0;
    starts[1] = XDIM/2;

    MPI_Type_create_subarray(NDIMS, sizes, sub, starts, MPI_ORDER_C, MPI_INT,
            &subarray);
    MPI_Type_commit(&subarray);

    MPI_CHECK(MPI_File_open(comm, filename, MPI_MODE_RDONLY, info, &fh));
    MPI_CHECK(MPI_File_set_view(fh, 0, MPI_INT, subarray, "native", info));
    MPI_Type_free(&subarray);
    MPI_CHECK(MPI_File_read_all(fh, read_buf, nprocs, MPI_INT, MPI_STATUS_IGNORE));

    MPI_CHECK(MPI_File_close(&fh));
    MPI_Info_free(&info);

    if (!rank) {
        for (i=0; i<nprocs; i++)
            printf("%d ", read_buf[i]);
    }
    printf("\n");


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
