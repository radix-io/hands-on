/* SLIDE: 2D Life Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>

#include "mlife2d.h"

static int exch_above, exch_below, exch_left, exch_right, 
    above_LRows, left_LCols,  right_LCols;

static MPI_Win matrix_win, temp_win;

typedef struct mem_win{
    void *mem;
    MPI_Win win;
} mem_win;

static mem_win mem_win_map[2];
       /* SLIDE: 2D Life Code Walkthrough */
int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp,
			int rows, int cols, int LRows, int LCols,
                        int above, int below, int left, int right)
{
    int      err=MPI_SUCCESS;
    int      tmp_next, tmp_prev, tmp_left, tmp_right;
    int      tmp_LRows, tmp_LCols; 
    int      tmp_GFirstRow, tmp_GFirstCol; 
    int      nprocs;

    exch_above = above;
    exch_below = below;
    exch_left  = left;
    exch_right = right;

    /* create windows */
    MPI_Win_create(matrix, (LRows+2)*(LCols+2)*sizeof(int), 
                   sizeof(int), MPI_INFO_NULL, comm, &matrix_win);

    MPI_Win_create(temp, (LRows+2)*(LCols+2)*sizeof(int), 
                   sizeof(int), MPI_INFO_NULL, comm, &temp_win);

    /* store the mapping from memory address to associated 
       window */ 

    mem_win_map[0].mem = matrix;
    mem_win_map[0].win = matrix_win;
    mem_win_map[1].mem = temp;
    mem_win_map[1].win = temp_win;


/* SLIDE: 2D Life Code Walkthrough */
    /* for one-sided communication, we need to know the number of
       local rows in rank exch_above and the number of local 
       columns in rank exch_left and exch_right in order to do 
       the puts into the right locations in memory. */
    
    MPI_Comm_size(comm, &nprocs);

    if (exch_above == MPI_PROC_NULL)
        above_LRows = 0;
    else {
        MLIFE_MeshDecomp(exch_above, nprocs, 
                      rows, cols,
                      &tmp_left, &tmp_right, &tmp_prev, &tmp_next,
                      &above_LRows, &tmp_LCols, &tmp_GFirstRow, 
                      &tmp_GFirstCol);
    }

    if (exch_left == MPI_PROC_NULL)
        left_LCols = 0;
    else {
        MLIFE_MeshDecomp(exch_left, nprocs, 
                      rows, cols,
                      &tmp_left, &tmp_right, &tmp_prev, &tmp_next,
                      &tmp_LRows, &left_LCols, &tmp_GFirstRow, 
                      &tmp_GFirstCol);
    }

    if (exch_right == MPI_PROC_NULL)
        right_LCols = 0;
    else {
        MLIFE_MeshDecomp(exch_right, nprocs, 
/* SLIDE: 2D Life Code Walkthrough */
                      rows, cols,
                      &tmp_left, &tmp_right, &tmp_prev, &tmp_next,
                      &tmp_LRows, &right_LCols, &tmp_GFirstRow,
                      &tmp_GFirstCol);
    }

    return err;
}

void MLIFE_exchange_finalize(void)
{
    MPI_Win_free(&matrix_win);
    MPI_Win_free(&temp_win);
}
       /* SLIDE: 2D Life Code Walkthrough */
int MLIFE_exchange(int **matrix,
		   int LRows,
		   int LCols)
{
    int err=MPI_SUCCESS;
    MPI_Aint disp;
    static MPI_Datatype mytype = MPI_DATATYPE_NULL;
    static MPI_Datatype left_type = MPI_DATATYPE_NULL;
    static MPI_Datatype right_type = MPI_DATATYPE_NULL;

    MPI_Win win;

    /* Find the right window object */
    if (mem_win_map[0].mem == &matrix[0][0])
        win = mem_win_map[0].win;
    else
        win = mem_win_map[1].win;

    /* create datatype if not already created */
    if (mytype == MPI_DATATYPE_NULL) {
	MPI_Type_vector(LRows, 1, LCols+2, MPI_INT, &mytype);
        MPI_Type_commit(&mytype);
    }
    if (left_type == MPI_DATATYPE_NULL) {
	MPI_Type_vector(LRows, 1, left_LCols+2, MPI_INT, &left_type);
        MPI_Type_commit(&left_type);
    }
    if (right_type == MPI_DATATYPE_NULL) {
	MPI_Type_vector(LRows, 1, right_LCols+2, MPI_INT,
                        &right_type);
/* SLIDE: 2D Life Code Walkthrough */
        MPI_Type_commit(&right_type);
    }


    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);

    /* first put the left, right edges */

    disp = (left_LCols + 2) + (left_LCols + 1);
    MPI_Put(&matrix[1][1], 1, mytype, exch_left, disp, 1, 
	    left_type, win);

    disp = right_LCols + 2;
    MPI_Put(&matrix[1][LCols], 1, mytype, exch_right, disp, 1, 
            right_type, win); 

    /* Complete the right/left transfers for the diagonal trick */
    MPI_Win_fence( 0, win );

    /* now put the top, bottom edges (including the diagonal 
       points) */
    MPI_Put(&matrix[1][0], LCols + 2, MPI_INT, exch_above, 
            (above_LRows+1)*(LCols+2), LCols+2, MPI_INT, win);

    MPI_Put(&matrix[LRows][0], LCols + 2, MPI_INT, exch_below, 0, 
            LCols+2, MPI_INT, win);

    MPI_Win_fence(MPI_MODE_NOSTORE | MPI_MODE_NOPUT | 
                  MPI_MODE_NOSUCCEED, win);
    return err;
}
