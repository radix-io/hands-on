/* SLIDE: Point-to-Point Exchange */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>

#include "mlife.h"

static MPI_Comm exch_comm = MPI_COMM_NULL;
static int exch_prev, exch_next;

/* MLIFE_exchange_init
 *
 * Parameters:
 * comm   - communicator describing group of processes
 * matrix - pointer to original matrix data
 * temp   - pointer to second region used during calculations
 * myrows - number of rows held locally
 * rows   - number of rows in global matrix
 * cols   - number of columns in global matrix
 * prev   - rank of processor "above" this one
 * next   - rank of processor "below" this one
 *
 * Note: Some of these parameters are not used by this exchange
 *       implementation.
 */
int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp,
                        int myrows, int rows, int cols,
                        int prev, int next)
{
/* SLIDE: Point-to-Point Exchange */
    MPI_Comm_dup(comm, &exch_comm);
    exch_prev = prev;
    exch_next = next;

    return MPI_SUCCESS;
}

void MLIFE_exchange_finalize(void)
{
    MPI_Comm_free(&exch_comm);
}

       /* SLIDE: Point-to-Point Exchange */
int MLIFE_exchange(int **matrix, int myrows, int cols)
{
    MPI_Request reqs[4];

    /* exchange edges */
    MPI_Isend(&matrix[1][0], cols+2, MPI_INT,
	      exch_prev, 0, exch_comm, reqs);
    MPI_Irecv(&matrix[0][0], cols+2, MPI_INT,
	      exch_prev, 0, exch_comm, reqs+1);
    MPI_Isend(&matrix[myrows][0], cols+2, MPI_INT,
	      exch_next, 0, exch_comm, reqs+2);
    MPI_Irecv(&matrix[myrows+1][0], cols+2, MPI_INT,
	      exch_next, 0, exch_comm, reqs+3);

    MPI_Waitall(4, reqs, MPI_STATUSES_IGNORE);

    return MPI_SUCCESS;
}

/* END OF EXAMPLE */
