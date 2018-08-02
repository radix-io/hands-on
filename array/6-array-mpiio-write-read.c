#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "array.h"
#include "util.h"
#include "mpi-util.h"

int write_data(MPI_Comm comm, char *filename)
{
    MPI_File fh;
    MPI_Info info;
    int *values;
    int rank, nprocs;
    science header;


    MPI_Info_create(&info);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

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

int read_data(MPI_Comm comm, char *filename)
{
    MPI_File fh;
    MPI_Info info;
    int *read_buf;
    int rank, nprocs;
    MPI_Datatype subarray;
    int sizes[NDIMS], sub[NDIMS], starts[NDIMS];
    int i;
    science header;

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

    /* HANDS-ON: create a datatype with MPI_Type_create_subarray.  The size of
     * the array is {nprocs, XDIM}".  The subarray size is "{nprocs,1}".  The
     * starting coordinates are "{0, XDIM/2}".
     *
     * Use MPI_ORDER_C and the underlying type will be MPI_INT */

    MPI_Type_commit(&subarray);

    MPI_CHECK(MPI_File_open(comm, filename, MPI_MODE_RDONLY, info, &fh));

    /* Here's how rank 0 reads the metadata */
    if (!rank) {
        MPI_CHECK(MPI_File_read(fh,
                    &header, sizeof(header), MPI_BYTE,
                    MPI_STATUS_IGNORE) );
    }

    /* HANDS-ON: set the file view with the type you created above.  You should
     * skip over the header -- it is "sizeof(header)" bytes big. */

    MPI_Type_free(&subarray);

    /* HANDS-ON: collectively read with MPI_File_read_all.  How much data
     * should you read?  'read_buf' was allocated above and holds 'nprocs'
     * integers (MPI_INT) .  It's ok to use MPI_STATUS_IGNORE */

    MPI_CHECK(MPI_File_close(&fh));
    MPI_Info_free(&info);

    if (!rank) {
        printf("array is %d by %d ; experiment on iteration %d\n",
                header.row, header.col, header.iter);
        for (i=0; i<nprocs; i++)
            printf("%d ", read_buf[i]);
	printf("\n");
    }


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
