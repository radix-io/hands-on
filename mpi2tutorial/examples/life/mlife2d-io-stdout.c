/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mpi.h>

#include "mlife2d.h"
#include "mlife-io.h"

/* stdout implementation of checkpoint (no restart) for MPI Life
 *
 * Data output in matrix order: spaces represent dead cells,
 * '*'s represent live ones.
 */

static void MLIFEIO_Row_print(int *data, int dimsz, int rownr,
                              int labelRow);
static void MLIFEIO_msleep(int msec);

static MPI_Comm mlifeio_comm = MPI_COMM_NULL;


int MLIFEIO_Init(MPI_Comm comm)
{
    int err;

    err = MPI_Comm_dup(comm, &mlifeio_comm);

    return err;
}

int MLIFEIO_Finalize(void)
{
    int err = MPI_SUCCESS;
    
    if (mlifeio_comm != MPI_COMM_NULL) {
        err = MPI_Comm_free(&mlifeio_comm);
    }

    return err;
}


int MLIFEIO_Checkpoint(char *prefix, int **matrix,
                       int GRows, int GCols,
                       int iter, MPI_Info info)
{
    int err = 0;
    int rank, nprocs;
    int r;
    int LRows, LCols;
    int GFirstRow, GFirstCol;

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    MLIFE_MeshDecomp(rank, nprocs, GRows, GCols, 
                     NULL, NULL, NULL, NULL, 
                     &LRows, &LCols, &GFirstRow, &GFirstCol);

    /* each proc. writes its part of the display, in rank order */
    if (rank == 0) {
        printf("[H[2J# Iteration %d\n", iter);
    }

    /* Slow but simple ... */
    for (r=0; r<nprocs; r++) {
        int i;

        if (rank == r) {
            /* print rank 0 data first */
            for (i=1; i <= LRows; i++) {
                if (GFirstCol == 1) {
                    printf("[%03d;%03dH", 1+(i-1+GFirstRow+1),
                           GFirstCol);
                }
                else {
                    printf("[%03d;%03dH", 1+(i-1+GFirstRow+1),
                           GFirstCol+5);
                }
                MLIFEIO_Row_print(&matrix[i][1], LCols,
                                  i+GFirstRow-1, GFirstCol==1);
            }
            fflush(stdout);
        }
        MPI_Barrier(mlifeio_comm);
    }
    
    if (rank == nprocs-1) {
        printf("[%03d;%03dH", GRows+3, 1);
        fflush(stdout);
    }

    MLIFEIO_msleep(250); /* give time to see the results */

    return err;
}

static void MLIFEIO_Row_print(int *data, int cols, int rownr,
                              int labelRow)
{
    int i;

    if (labelRow) { printf("%3d: ", rownr); }
    for (i=0; i < cols; i++) {
        printf("%c", (data[i] == BORN) ? '*' : ' ');
    }
    printf("\n");
}

int MLIFEIO_Restart(char *prefix, int **matrix,
                    int GRows, int GCols,
                    int iter, MPI_Info info)
{
    return MPI_ERR_IO;
}

int MLIFEIO_Can_restart(void)
{
    return 0;
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
