/*
 * (C) 2014 The University of Chicago.
 */

/* COMPILE:
 *
 * cc -Wall helloworld.c -o helloworld
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

#define IOSIZE (1*1024*1024)
#define IOITS  6

static int example1A(const char* dir, int rank, int nprocs);

int main(int argc, char **argv)
{
    char* dir;
    int ret;
    int rank;
    int nprocs;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if(argc != 2)
    {
        if(rank == 0)
        {
            fprintf(stderr, "Usage: helloworld <directory>\n");
            MPI_Finalize();
            return(-1);
        }
    }

    dir = strdup(argv[1]);

    /*****************/
    sleep(1);
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) printf("  Running...\n");
    ret = example1A(dir, rank, nprocs);
    if(ret < 0)
    {
        if(rank == 0) fprintf(stderr, "Failure.\n");
        MPI_Finalize();
        return(-1);
    }
    if(rank == 0) printf("  Completed.\n");

    free(dir);
    MPI_Finalize();
    
    return(0);
}

static int example1A(const char* dir, int rank, int nprocs)
{
    char file_name[PATH_MAX];
    int ret;
    MPI_File fh;
    char msg[MPI_MAX_ERROR_STRING];
    int msg_len;
    size_t buffer_size;
    char *buffer;
    MPI_Status status;
    size_t io_size = IOSIZE;
    MPI_Offset phase_offset, my_offset;
    MPI_Offset i;

    buffer_size = IOSIZE * (1 << (IOITS - 1));
    buffer = malloc(buffer_size);

    sprintf(file_name, "%s/helloworld", dir);

    ret = MPI_File_open(MPI_COMM_WORLD, file_name, 
        MPI_MODE_CREATE |MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    if(ret != MPI_SUCCESS)
    {
        MPI_Error_string(ret, msg, &msg_len);
        fprintf(stderr, "Error: MPI_File_open: %s: %s\n", file_name, msg);
        free(buffer);
        return(-1);
    }

    phase_offset = 0;
    for(i=0; i<IOITS; i++)
    {
        /* skip ahead to offset for this process' data */
        my_offset = phase_offset + (MPI_Offset)rank*io_size;
        ret = MPI_File_write_at_all(fh, my_offset, buffer, io_size, MPI_CHAR,
            &status);
        //fprintf(stderr, "rank %d: writing %lu @ %lu\n", rank, io_size, my_offset);
        if(ret != MPI_SUCCESS)
        {
            MPI_Error_string(ret, msg, &msg_len);
            fprintf(stderr, "Error: MPI_File_write_at_all: %s: %s\n", file_name, msg);
            free(buffer);
            return(-1);
        }

        /* sleep between iterations */
        sleep(1);

        /* skip ahead to next I/O phase offset */
        phase_offset += (MPI_Offset)nprocs*io_size;

        /* double access size for next iteration */
        io_size *= 2;
    }

    ret = MPI_File_close(&fh);
    if(ret != MPI_SUCCESS)
    {
        MPI_Error_string(ret, msg, &msg_len);
        fprintf(stderr, "Error: MPI_File_close: %s: %s\n", file_name, msg);
        free(buffer);
        return(-1);
    }

    free(buffer);
    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 *
 * vim: ts=4
 * End:
 */ 


