#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void handle_error(int errcode, char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

#define MPI_CHECK(fn) { int errcode; errcode = (fn); if (errcode != MPI_SUCCESS) handle_error(errcode, #fn ); }
#define BUFSIZE 256

int main(int argc, char **argv)
{
    MPI_File fh;
    int rank, nprocs;
    MPI_Info info;
    MPI_Status status;
    MPI_Offset offset=0, len;
    char buf[BUFSIZE];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* an "Info object":  these store key-value strings for tuning the
     * underlying MPI-IO implementation */
    MPI_Info_create(&info);

    /* we can deal with variable length strings but for the sake of
     * this exercise it will be simpler if all strings are the same
     * length */
    snprintf(buf, BUFSIZE, "Hello from rank %3d of %3d\n", rank,
            nprocs);
    len = strlen(buf);

    /* noncontiguous in file requres a "file view".  In this example
     * we will have each process write their strings such that all
     * the 'H' comes first, then all the 'e' ... etc
     * so two processes would write HHeelloo... */
    MPI_Datatype viewtype;
    int *displacements;
    displacements = malloc(len*sizeof(*displacements));

    /* each process will write to its own "view" of the file: for two
     * processes it would look like:
     * - - - - - - - - - - - ...
     * then the write would fill in those locations like this:
     * Rank 0:
     * H e l l o   f r o m  ...
     * Rank 1:
     *  H e l l o  f r o m ...
     *
     * and we set up displacements so that each process does not step
     * on any other process: in the file it looks like
     * HHeelloo  ffrroomm... */
    for (int i=0; i< len; i++)
        displacements[i] = rank+(i*nprocs);
    MPI_Type_create_indexed_block(len, 1, displacements, MPI_CHAR, &viewtype);
    MPI_Type_commit(&viewtype);
    free(displacements);

    MPI_CHECK(MPI_File_open(MPI_COMM_WORLD, argv[1],
                MPI_MODE_CREATE|MPI_MODE_WRONLY, info, &fh));
    MPI_CHECK(MPI_File_set_view(fh, 0, MPI_CHAR, viewtype, "native", info));

    /* _all means collective.  Even if we had no data to write, we
     * would still have to make this call.  In exchange for this
     * coordination, the underlyng library might be able to greatly
     * optimize the I/O */
    MPI_CHECK(MPI_File_write_at_all(fh, offset, buf, len, MPI_CHAR,
                &status));

    MPI_CHECK(MPI_File_close(&fh));

    MPI_Type_free(&viewtype);
    MPI_Info_free(&info);
    MPI_Finalize();
}
