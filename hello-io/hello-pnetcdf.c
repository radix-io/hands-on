#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <pnetcdf.h>

static void handle_error(int status, char *str, int lineno)
{
    fprintf(stderr, "%s: Error at line %d: %s\n", str, lineno, ncmpi_strerror(status));
    MPI_Abort(MPI_COMM_WORLD, 1);
}

#define NC_CHECK(fn) { int errcode; errcode = (fn); if (errcode != NC_NOERR) handle_error(errcode, #fn, __LINE__); }

#define BUFSIZE 256

int main(int argc, char **argv)
{
    int nprocs, rank;
    int ncfile, dimid, varid;
    char buf[BUFSIZE];

    MPI_Info info;
    MPI_Offset offset=0, len, varlen;


    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Info_create(&info);

    snprintf(buf, BUFSIZE, "Hello from rank %d of %d\n", rank, nprocs);
    len = strlen(buf);

    /* each process figures out where to write its string */
    MPI_Exscan(&len, &offset, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);

    /* and then the last process tells everyone else how big it ended up being */
    varlen = offset+len;
    MPI_Bcast(&varlen, 1, MPI_OFFSET, nprocs-1, MPI_COMM_WORLD);

    NC_CHECK(ncmpi_create(MPI_COMM_WORLD, argv[1],
		NC_CLOBBER|NC_64BIT_OFFSET, MPI_INFO_NULL, &ncfile));

    /* just one big string in this silly example */
    NC_CHECK(ncmpi_def_dim(ncfile, "d1", varlen, &dimid));
    NC_CHECK(ncmpi_def_var(ncfile, "v1", NC_CHAR, 1, &dimid, &varid));

    NC_CHECK(ncmpi_enddef(ncfile));

    NC_CHECK(ncmpi_put_vara_text_all(ncfile, varid, &offset, &len, buf));

    NC_CHECK(ncmpi_close(ncfile));

    MPI_Finalize();

    return 0;
}
