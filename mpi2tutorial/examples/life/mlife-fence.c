/* SLIDE: Fence Life Exchange Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>

#include "mlife.h"

static int exch_prev, exch_next, nrows_prev;
static MPI_Win matrix_win, temp_win;

typedef struct mem_win{
    void *mem;
    MPI_Win win;
} mem_win;

static mem_win mem_win_map[2];

int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp,
                        int myrows, int rows, int cols, int prev,
			int next)
{
    int err=MPI_SUCCESS, nprocs;

    exch_prev = prev;
    exch_next = next;

    /* create windows */
/* SLIDE: Fence Life Exchange Code Walkthrough */
    MPI_Win_create(matrix, (myrows+2)*(cols+2)*sizeof(int), 
                   sizeof(int), MPI_INFO_NULL, comm, &matrix_win);

    MPI_Win_create(temp, (myrows+2)*(cols+2)*sizeof(int), 
                   sizeof(int), MPI_INFO_NULL, comm, &temp_win);

    /* store mapping from memory address to associated window */

    mem_win_map[0].mem = matrix;
    mem_win_map[0].win = matrix_win;
    mem_win_map[1].mem = temp;
    mem_win_map[1].win = temp_win;

    /* calculate no. of local rows in prev */
    
    if (exch_prev == MPI_PROC_NULL)
        nrows_prev = 0;
    else {
        MPI_Comm_size(comm, &nprocs);
        nrows_prev = MLIFE_myrows(rows, exch_prev, nprocs); 
    }

    return err;
}

void MLIFE_exchange_finalize(void)
{
    MPI_Win_free(&matrix_win);
    MPI_Win_free(&temp_win);
}
       /* SLIDE: Fence Life Exchange Code Walkthrough */
int MLIFE_exchange(int **matrix, int myrows, int cols)
{
    int err=MPI_SUCCESS;
    MPI_Win win;

    /* Find the right window object */
    if (mem_win_map[0].mem == &matrix[0][0])
        win = mem_win_map[0].win;
    else
        win = mem_win_map[1].win;

    /* Send and receive boundary information */
    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);

    MPI_Put(&matrix[myrows][0], cols+2, MPI_INT, exch_next, 0,
            cols+2, MPI_INT, win);

    MPI_Put(&matrix[1][0], cols+2, MPI_INT, exch_prev, 
      (nrows_prev+1)*(cols+2), cols+2, MPI_INT, win);

    MPI_Win_fence(MPI_MODE_NOSTORE | MPI_MODE_NOPUT |
		  MPI_MODE_NOSUCCEED, 
                  win);

    return err;
}
