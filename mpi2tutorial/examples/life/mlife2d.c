/* SLIDE: 2D Life Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mpi.h>

#include "mlife2d.h"
#include "mlife-io.h"

static int MLIFE_nextstate(int **matrix, int y, int x);
static int MLIFE_parse_args(int argc, char **argv);

/* options */
static int opt_rows = 25, opt_cols = 70, opt_iter = 10;
static int opt_prows = 0, opt_pcols = 0, opt_restart_iter = -1;
int npcols = 0;
static char opt_prefix[64] = "mlife";

       /* SLIDE: 2D Life Code Walkthrough */
int main(int argc, char *argv[])
{
    int rank;
    double time;
  
    MPI_Init(&argc, &argv);
    MLIFE_parse_args(argc, argv);
    MLIFEIO_Init(MPI_COMM_WORLD);

    time = life(opt_rows, opt_cols, opt_iter, MPI_COMM_WORLD);

    /* Print the total time taken */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
        printf("[%d] Life finished in %lf secs of calculation\n",
               rank, time );

    MLIFEIO_Finalize();
    MPI_Finalize();

    return 0;
}

       /* SLIDE: 2D Life Code Walkthrough */
double life(int rows, int cols, int ntimes, MPI_Comm comm)
{
    int      rank, nprocs;
    int      next, prev, left, right;
    int      i, j, k;
    int    **matrix, **temp, **addr;
    int     *matrixData, *tempData;
    double   slavetime, totaltime, starttime;

    /* count of local rows/columns excluding halo */
    int      LRows, LCols;

    /* index of first row and col of local mesh in the global
     * mesh relative to the [1][1] entry of local mesh
     */
    int      GFirstRow, GFirstCol;

    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    /* set neighbors and determine my part of the matrix,
     * block-block distribution
     */
    MLIFE_MeshDecomp(rank, nprocs, rows, cols,
                     &left, &right, &prev, &next, 
                     &LRows, &LCols, &GFirstRow, &GFirstCol);

    /* allocate memory for the matrix using single blocks */
    matrix     = (int **) malloc((LRows+2) * sizeof(int *));
    temp       = (int **) malloc((LRows+2) * sizeof(int *));
    matrixData = (int *) malloc((LRows+2)*(LCols+2)*sizeof(int));
/* SLIDE: 2D Life Code Walkthrough */
    tempData   = (int *) malloc((LRows+2)*(LCols+2)*sizeof(int));

    /* set up pointers for convenience */
    matrix[0] = matrixData;
    temp[0]   = tempData;
    for (i = 1; i < LRows+2; i++) {
        matrix[i] = matrix[i-1] + LCols + 2;
        temp[i]   = temp[i-1]   + LCols + 2;
    }

    /* initialize the boundaries of the life matrix */
    for (j = 0; j < LCols+2; j++) {
        matrix[0][j] = matrix[LRows+1][j] = temp[0][j]
                     = temp[LRows+1][j] = DIES;
    }
    for (i = 0; i < LRows+2; i++) {
        matrix[i][0] = matrix[i][LCols+1] = temp[i][0]
                     = temp[i][LCols+1] = DIES;
    }

    /* Initialize the life matrix */
    for (i = 1; i <= LRows; i++)  {
        srand48((long)(1000^(i + GFirstRow-1)));
        /* advance to the random number generator to the
	 * first *owned* cell
	 */
        for (j=1; j<GFirstCol; j++) {    
            (void)drand48();
        }

        for (j = 1; j<= LCols; j++)
/* SLIDE: 2D Life Code Walkthrough */
            if (drand48() > 0.5) matrix[i][j] = BORN;
            else                 matrix[i][j] = DIES;
    }

    MLIFE_exchange_init(comm, &matrix[0][0], &temp[0][0], rows,
			cols, LRows, LCols, prev, next, left,
			right);

    /* use portable MPI function for timing */
    starttime = MPI_Wtime();

    for (k = 0; k < ntimes; k++)
    {
        MLIFE_exchange(matrix, LRows, LCols);

        /* calculate new state for all non-boundary elements */
        for (i = 1; i <= LRows; i++) {
            for (j = 1; j < LCols+1; j++) {
                temp[i][j] = MLIFE_nextstate(matrix, i, j);
            }
        }

        /* swap the matrices */
        addr   = matrix;
        matrix = temp;
        temp   = addr;

        MLIFEIO_Checkpoint(opt_prefix, matrix, rows, cols, 
                           k, MPI_INFO_NULL);
    }

/* SLIDE: 2D Life Code Walkthrough */
    /* return the average time taken/processor */
    slavetime = MPI_Wtime() - starttime;
    MPI_Reduce(&slavetime, &totaltime, 1, MPI_DOUBLE, MPI_SUM, 0,
               comm);

    MLIFE_exchange_finalize();
    free(matrix);
    free(temp);
    free(matrixData);
    free(tempData);

    return(totaltime/(double) nprocs);
}

       /* SLIDE: 2D Life Code Walkthrough */
/* MLIFE_MeshDecomp
 *
 * Compute coordinates of this patch, given the rank and size of
 * the global mesh.  Also return the neighbors.
 */
void MLIFE_MeshDecomp(int rank, int nprocs, int GRows, int GCols,
                      int *leftP, int *rightP,
		      int *topP, int *bottomP, 
                      int *LRowsP, int *LColsP, 
                      int *GFirstRowP, int *GFirstColP)
{
    int dims[2];
    int top, bottom, left, right;
    int prow, pcol;
    int firstrow, lastrow, firstcol, lastcol;

    dims[0] = opt_prows;
    dims[1] = opt_pcols;

    MPI_Dims_create(nprocs, 2, dims);

    /* Remember the number of processes across */
    npcols = dims[1];

    /* compute the cartesian coords of this process; number across
     * rows changing column by 1 changes rank by 1)
     */
    prow = rank / dims[1];
    pcol = rank % dims[1];
    
    /* Compute the neighbors */
/* SLIDE: 2D Life Code Walkthrough */
    left = right = top = bottom = MPI_PROC_NULL;
    if (prow > 0) {
        top = rank - dims[1];
    }
    if (pcol > 0) {
        left = rank - 1;
    }
    if (prow < dims[0]-1) {
        bottom = rank + dims[1];
    }
    if (pcol < dims[1]-1) {
        right = rank + 1;
    }
    if (leftP) {
        /* allow a NULL for leftP to suppress all of the neighbor
         * information
	 */
        *leftP = left; *rightP = right;
	*topP = top; *bottomP = bottom;
    }
    
    /* compute the decomposition of the global mesh.
     * these are for the "active" part of the mesh, and range from
     * 1 to GRows by 1 to GCols
     */
    firstcol = 1 + pcol * (GCols / dims[1]);
    firstrow = 1 + prow * (GRows / dims[0]);
    if (pcol == dims[1] - 1) {
        lastcol = GCols;
    }
    else {
/* SLIDE: 2D Life Code Walkthrough */
        lastcol  = 1 + (pcol + 1) * (GCols / dims[1]) - 1;
    }
    if (prow == dims[0] - 1) {
        lastrow = GRows;
    }
    else {
        lastrow = 1 + (prow + 1) * (GRows / dims[0]) - 1;
    }

    *LRowsP     = lastrow - firstrow + 1;
    *LColsP     = lastcol - firstcol + 1;
    *GFirstRowP = firstrow;
    *GFirstColP = firstcol;
}

       /* SLIDE: 2D Life Code Walkthrough */
static int MLIFE_nextstate(int **matrix,
                           int row,
                           int col)
{
    int sum;

    /* add values of all eight neighbors */
    sum = matrix[row-1][col-1] + matrix[row-1][col] +
          matrix[row-1][col+1] + matrix[row][col-1] +
          matrix[row][col+1] + matrix[row+1][col-1] +
          matrix[row+1][col] + matrix[row+1][col+1];

    if (sum < 2 || sum > 3) return DIES;
    else if (sum == 3)      return BORN;
    else                    return matrix[row][col];
}

       /* SLIDE: 2D Life Code Walkthrough */
/* MLIFE_parse_args
 *
 * Note: Command line arguments are not guaranteed in the MPI
 *       environment to be passed to all processes.  To be
 *       portable, we must process on rank 0 and distribute
 *       results.
 */
static int MLIFE_parse_args(int argc, char **argv)
{
    int ret;
    int rank;
    int myargs[7]; /* array for simple sending of arguments */

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        while ((ret = getopt(argc, argv, "a:b:x:y:i:p:r:")) >= 0)
        {
            switch(ret) {
                case 'a':
                    opt_pcols = atoi(optarg);
                    break;
                case 'b':
                    opt_prows = atoi(optarg);
                    break;
                case 'x':
                    opt_cols = atoi(optarg);
                    break;
                case 'y':
                    opt_rows = atoi(optarg);
                    break;
/* SLIDE: 2D Life Code Walkthrough */
                case 'i':
                    opt_iter = atoi(optarg);
                    break;
                case 'r':
                    opt_restart_iter = atoi(optarg);
                case 'p':
                    strncpy(opt_prefix, optarg, 63);
                    break;
                default:
                    break;
            }
        }

        myargs[0] = opt_rows;
        myargs[1] = opt_cols;
        myargs[2] = opt_iter;
        myargs[3] = opt_restart_iter;
        myargs[4] = strlen(opt_prefix) + 1;
        myargs[5] = opt_prows;
        myargs[6] = opt_pcols;
    }

    MPI_Bcast(myargs, 7, MPI_INT, 0, MPI_COMM_WORLD);
    opt_rows = myargs[0];
    opt_cols = myargs[1];
    opt_iter = myargs[2];
    opt_prows = myargs[5];
    opt_pcols = myargs[6];

    MPI_Bcast(opt_prefix, myargs[4], MPI_CHAR, 0, MPI_COMM_WORLD);

/* SLIDE: 2D Life Code Walkthrough */
    return 0;
}

