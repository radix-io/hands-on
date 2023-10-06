#include <mpi.h>
#include <stdio.h>
#include <string.h>

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

    snprintf(buf, BUFSIZE, "Hello from rank %d of %d\n", rank, nprocs);
    len = strlen(buf);
    /* We're working with strings here but this approach works well
     * whenever amounts of data vary from process to process. */
    MPI_Exscan(&len, &offset, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);

    MPI_CHECK(MPI_File_open(MPI_COMM_WORLD, argv[1],
                MPI_MODE_CREATE|MPI_MODE_WRONLY, info, &fh));

    /* _all means collective.  Even if we had no data to write, we would
     * still have to make this call.  In exchange for this coordination,
     * the underlyng library might be able to greatly optimize the I/O */
    MPI_CHECK(MPI_File_write_at_all(fh, offset, buf, len, MPI_CHAR,
                &status));

    MPI_CHECK(MPI_File_close(&fh));

    MPI_Info_free(&info);
    MPI_Finalize();
}
