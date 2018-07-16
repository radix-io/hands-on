/* SLIDE: stdio Life Checkpoint Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mpi.h>

#include "mlife.h"
#include "mlife-io.h"

/* stdout implementation of checkpoint (no restart) for MPI Life
 *
 * Data output in matrix order: spaces represent dead cells,
 * '*'s represent live ones.
 */

static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype);
static void MLIFEIO_Row_print(int *data, int cols, int rownr);
static void MLIFEIO_msleep(int msec);

static MPI_Comm mlifeio_comm = MPI_COMM_NULL;
       /* SLIDE: stdio Life Checkpoint Code Walkthrough */
int MLIFEIO_Init(MPI_Comm comm)
{
    int err;

    err = MPI_Comm_dup(comm, &mlifeio_comm);

    return err;
}

int MLIFEIO_Finalize(void)
{
    int err;

    err = MPI_Comm_free(&mlifeio_comm);

    return err;
}
       /* SLIDE: Life stdout "checkpoint" */
/* MLIFEIO_Checkpoint
 *
 * Parameters:
 * prefix - prefix of file to hold checkpoint (ignored)
 * matrix - data values
 * rows   - number of rows in matrix
 * cols   - number of columns in matrix
 * iter   - iteration number of checkpoint
 * info   - hints for I/O (ignored)
 *
 * Returns MPI_SUCCESS on success, MPI error code on error.
 */
int MLIFEIO_Checkpoint(char *prefix, int **matrix, int rows,
                       int cols, int iter, MPI_Info info)
{
    int err = MPI_SUCCESS, rank, nprocs, myrows, myoffset;
    MPI_Datatype type;

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    if (rank != 0) {
        /* send all data to rank 0 */

        MLIFEIO_Type_create_rowblk(matrix, myrows, cols, &type);
        MPI_Type_commit(&type);
        err = MPI_Send(MPI_BOTTOM, 1, type, 0, 1, mlifeio_comm);
        MPI_Type_free(&type);
/* SLIDE: Describing Data */
    }
    else {
        int i, procrows, totrows;

        printf("[H[2J# Iteration %d\n", iter);

        /* print rank 0 data first */
        for (i=1; i < myrows+1; i++) {
            MLIFEIO_Row_print(&matrix[i][1], cols, i);
        }
        totrows = myrows;

        /* receive and print others' data */
        for (i=1; i < nprocs; i++) {
                    int j, *data;

            procrows = MLIFE_myrows(rows, i, nprocs);
            data = (int *) malloc(procrows * cols * sizeof(int));

            err = MPI_Recv(data, procrows * cols, MPI_INT, i, 1,
                           mlifeio_comm, MPI_STATUS_IGNORE);

            for (j=0; j < procrows; j++) {
                MLIFEIO_Row_print(&data[j * cols], cols,
                                  totrows + j + 1);
            }
            totrows += procrows;

            free(data);
        }
    }
/* SLIDE: Describing Data */

    MLIFEIO_msleep(250); /* give time to see the results */

    return err;
}
       /* SLIDE: Describing Data */
/* MLIFEIO_Type_create_rowblk
 *
 * Creates a MPI_Datatype describing the block of rows of data
 * for the local process, not including the surrounding boundary
 * cells.
 *
 * Note: This implementation assumes that the data for matrix is
 *       allocated as one large contiguous block!
 */
static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype)
{
    int err, len;
    MPI_Datatype vectype;
    MPI_Aint disp;

    /* since our data is in one block, access is very regular! */
    err = MPI_Type_vector(myrows, cols, cols+2, MPI_INT,
                          &vectype);
    if (err != MPI_SUCCESS) return err;

    /* wrap the vector in a type starting at the right offset */
    len = 1;
    MPI_Get_address(&matrix[1][1], &disp);
    err = MPI_Type_create_hindexed(1,&len,&disp,vectype,newtype);

    MPI_Type_free(&vectype); /* decrement reference count */

    return err;
}

static void MLIFEIO_Row_print(int *data, int cols, int rownr)
{
    int i;

    printf("%3d: ", rownr);
    for (i=0; i < cols; i++) {
        printf("%c", (data[i] == BORN) ? '*' : ' ');
    }
    printf("\n");
}

int MLIFEIO_Can_restart(void)
{
    return 0;
}

int MLIFEIO_Restart(char *prefix, int **matrix, int rows,
                    int cols, int iter, MPI_Info info)
{
    return MPI_ERR_IO;
}
       
#ifdef HAVE_NANOSLEEP
#include <time.h>
static void MLIFEIO_msleep(int msec)
{
    struct timespec t;

    
    t.tv_sec = msec / 1000;
    t.tv_nsec = 1000000 * (msec - t.tv_sec);

    nanosleep(&t, NULL);
}
#else
static void MLIFEIO_msleep(int msec)
{
    if (msec < 1000) {
        sleep(1);
    }
    else {
        sleep(msec / 1000);
    }
}
#endif
