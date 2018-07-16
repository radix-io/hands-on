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
    int err;
    
    if (mlifeio_comm != MPI_COMM_NULL) {
        err = MPI_Comm_free(&mlifeio_comm);
    }

    return err;
}


#define MAX_SIZE 256
int MLIFEIO_Checkpoint(char *prefix, int **matrix,
                       int GRows, int GCols,
                       int iter, MPI_Info info)
{
    int err = 0;
    int rank, nprocs;
    int LRows, LCols;
    int GFirstRow, GFirstCol;

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    MLIFE_MeshDecomp(rank, nprocs, GRows, GCols, 
                     NULL, NULL, NULL, NULL, 
                     &LRows, &LCols, &GFirstRow, &GFirstCol);

    /* To ensure that there are no errors in updating the display, 
       the data is sent to process 0 who writes it to the stdout.
       To avoid having the root process keep a copy of the entire 
       (global) array, we step through the global rows, with
       each process sending information about its row (using the 
       tag for the row).
    */
    if (rank == 0) {
	char cbuf[MAX_SIZE];
	int  buf[MAX_SIZE];
	int  np, row, i;
	extern int npcols;

        printf("[H[2J# Iteration %d\n", iter );
	for (row=1; row <= GRows; row++) {
	    /* Clear the cbuf */
	    for (i=0; i<GCols; i++) { 
		cbuf[i] = ' ';
	    }
	    /* We know how many processes there are in each row */
	    np = 0;
	    /* Does this process contribute to this row? */
	    if (row >= GFirstRow && row < GFirstRow + LRows) {
		for (i=0; i<LCols; i++) {
		    cbuf[i+GFirstCol-1] = 
			matrix[row-GFirstRow+1][i+1] ? '*' : ' ';
		}
		np ++;
	    }
	    while (np < npcols) {
		MPI_Status status;

		MPI_Recv( buf, MAX_SIZE, MPI_INT, MPI_ANY_SOURCE, row, 
			  mlifeio_comm, &status );
		/* For each entry in buf that is set, set the
		   corresponding element in cbuf.  buf[0] is the
		   first col index, buf[1] is the number of elements */
		for (i=0; i<buf[1]; i++) {
		    cbuf[buf[0]-1+i] = buf[i+2] ? '*' : ' ';
		}
		np ++;
	    }
	    cbuf[GCols] = 0;
	    printf("[%03d;%03dH%3d: %s", row+1, 1, row, cbuf );
	}
        printf("[%03d;%03dH", GRows+2, 1);
        fflush(stdout);
    }
    else {
	int buf[MAX_SIZE], i, j, row;

	buf[0] = GFirstCol;
	buf[1] = LCols;
	for (i=1; i <= LRows; i++) {
	    row = i + GFirstRow - 1;
	    for (j=0; j<LCols; j++) {
		buf[2+j] = matrix[i][j+1];
	    }
	    MPI_Send( buf, LCols + 2, MPI_INT, 0, row, mlifeio_comm );
	}
    }

    MLIFEIO_msleep(250); /* give time to see the results */

    return err;
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
