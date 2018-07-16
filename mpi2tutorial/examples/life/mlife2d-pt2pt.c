/* SLIDE: 2D Life Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>

#include "mlife2d.h"

static MPI_Comm exch_comm = MPI_COMM_NULL;
static int exch_above, exch_below, exch_left, exch_right;

int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp,
                        int rows, int cols, int LRows, int LCols, 
                        int above, int below, int left, int right)
{
    MPI_Comm_dup(comm, &exch_comm);
    exch_above = above;
    exch_below = below;
    exch_left  = left;
    exch_right = right;

    return MPI_SUCCESS;
}

void MLIFE_exchange_finalize(void)
{
    MPI_Comm_free(&exch_comm);
/* SLIDE: 2D Life Code Walkthrough */
}

       /* SLIDE: 2D Life Code Walkthrough */
int MLIFE_exchange(int **matrix,
                   int LRows,
                   int LCols)
{
    MPI_Request reqs[4];
    static MPI_Datatype type = MPI_DATATYPE_NULL;

    /* Send and receive boundary information */

    if (type == MPI_DATATYPE_NULL) {
        MPI_Type_vector(LRows, 1, LCols+2, MPI_INT, &type);
        MPI_Type_commit(&type);
    }
    /* first, move the left, right edges */
    MPI_Isend(&matrix[1][1], 1, type,
	      exch_left, 0, exch_comm, reqs);
    MPI_Irecv(&matrix[1][0], 1, type,
	      exch_left, 0, exch_comm, reqs+1);
    MPI_Isend(&matrix[1][LCols], 1, type,
	      exch_right, 0, exch_comm, reqs+2);
    MPI_Irecv(&matrix[1][LCols+1], 1, type,
	      exch_right, 0, exch_comm, reqs+3);
    /* We need to wait on these for the trick that we use to move
       the diagonal terms to work */
    MPI_Waitall( 4, reqs, MPI_STATUSES_IGNORE );

    /* move the top, bottom edges (including diagonals) */
    MPI_Isend(&matrix[1][0], LCols+2, MPI_INT,
	      exch_above, 0, exch_comm, reqs);
    MPI_Irecv(&matrix[0][0], LCols+2, MPI_INT,
/* SLIDE: 2D Life Code Walkthrough */
	      exch_above, 0, exch_comm, reqs+1);
    MPI_Isend(&matrix[LRows][0], LCols+2, MPI_INT,
	      exch_below, 0, exch_comm, reqs+2);
    MPI_Irecv(&matrix[LRows+1][0], LCols+2, MPI_INT,
	      exch_below, 0, exch_comm, reqs+3);

    MPI_Waitall(4, reqs, MPI_STATUSES_IGNORE);

    return MPI_SUCCESS;
}
