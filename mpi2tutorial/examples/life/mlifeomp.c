/* SLIDE: Main Program with OpenMP */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include <omp.h>

#include "mlife.h"
#include "mlife-io.h"

int inline MLIFE_nextstate(int **matrix, int y, int x);
static int MLIFE_parse_args(int argc, char **argv);
double MLIFE_Sweep( int **matrix, int **temp, 
		    int myrows, int rows, int cols, int ntimes, 
		    char opt_prefix[64], MPI_Comm comm );

/* options */
static int opt_rows = 25, opt_cols = 70, opt_iter = 10;
static int opt_restart_iter = -1;
static char opt_prefix[64] = "mlife";

       /* SLIDE: Main Program with OpenMP */
int main(int argc, char *argv[])
{
    int rank;
    double time;
  
    MPI_Init(&argc, &argv);
    MLIFE_parse_args(argc, argv);
    MLIFEIO_Init(MPI_COMM_WORLD);

    time = life(opt_rows, opt_cols, opt_iter, MPI_COMM_WORLD);

    /* print the total time taken */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
        printf("[%d] Life finished in %lf secs of calculation\n",
               rank, time);

    MLIFEIO_Finalize();
    MPI_Finalize();

    return 0;
}

       /* SLIDE: Main Program with OpenMP */
double life(int rows, int cols, int ntimes, MPI_Comm comm)
{
    int      err, i, j, k, rank, nprocs, next, prev;
    int      myrows, myoffset;
    int    **matrix, **temp, **addr;
    int     *mdata, *tdata;
    double   mytime, totaltime=0.0, starttime;

    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    /* set neighbors */
    if (rank == 0) 
        prev = MPI_PROC_NULL;
    else
        prev = rank-1;
    if (rank == nprocs - 1)
        next = MPI_PROC_NULL;
    else
        next = rank+1;

    /* determine my part of the matrix, row-block distribution */
    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    /* allocate memory for the matrix using single blocks */
    matrix = (int **) malloc((myrows+2) * sizeof(int *));
    temp   = (int **) malloc((myrows+2) * sizeof(int *));
    mdata  = (int *) malloc((myrows+2) * (cols+2) * sizeof(int));
    tdata  = (int *) malloc((myrows+2) * (cols+2) * sizeof(int));
       /* SLIDE: Main Program with OpenMP */
    /* set up pointers for convenience */
    matrix[0] = mdata;
    temp[0]   = tdata;
    for (i = 1; i < myrows+2; i++) {
        matrix[i] = matrix[i-1] + cols + 2;
        temp[i]   = temp[i-1] + cols + 2;
    }

    /* Use this pattern (same schedule as used in the compute 
       loop) to have the "owning" thread be the first to touch 
       an element. */

#ifdef USE_FIRST_TOUCH
#pragma omp parallel for default(none) private(i,j) \
    firstprivate(myrows,cols) \
    shared(temp,matrix)
    for (i = 1; i <= myrows; i++) {
	for (j = 1; j < cols+1; j++) {
	    temp[i][j]   = DIES;
	    matrix[i][j] = DIES;
	}
    }
#endif

    /* initialize the boundaries of the life matrix */
    for (j = 0; j < cols+2; j++) {
        matrix[0][j] = matrix[myrows+1][j] = temp[0][j]
                     = temp[myrows+1][j] = DIES;
    }
    for (i = 0; i < myrows+2; i++) {
        matrix[i][0] = matrix[i][cols+1] = temp[i][0]
                     = temp[i][cols+1] = DIES;
    }

    if (opt_restart_iter == -1) {
        /* initialize the life matrix */
        for (i = 1; i <= myrows; i++)  {
            srand48((long)(1000^(i + myoffset)));
            
            for (j = 1; j<= cols; j++) {
                if (drand48() > 0.5) matrix[i][j] = BORN;
                else                 matrix[i][j] = DIES;
            }
        }
    }
       /* SLIDE: Main Program with OpenMP */
    else if (MLIFEIO_Can_restart()) {
        /* read state from checkpoint file */
        err = MLIFEIO_Restart(opt_prefix, matrix, rows, cols,
                              opt_restart_iter, MPI_INFO_NULL);
        if (err != MPI_SUCCESS) {
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_IO);
        }
    }
    else {
        fprintf(stderr, "Restart unsupported by I/O backend.\n");
        MPI_Abort(MPI_COMM_WORLD, MPI_ERR_IO);
    }

    MLIFE_exchange_init(comm, &matrix[0][0], &temp[0][0],
                        myrows, rows, cols, prev, next);

    MPI_Barrier( MPI_COMM_WORLD );
    /* use portable MPI function for timing */

    totaltime = MLIFE_Sweep( matrix, temp, myrows, rows, cols, 
			     ntimes, opt_prefix, comm );

    MLIFE_exchange_finalize();
    free(matrix);
    free(temp);
    free(mdata);
    free(tdata);

    return(totaltime/(double) nprocs);
}

/* SLIDE: Main Program with OpenMP */
int inline MLIFE_nextstate(int **matrix, int row, int col)
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

int MLIFE_myrows(int rows, int rank, int nprocs)
{
    int myrows;

    myrows = rows / nprocs;

    if (rank < (rows % nprocs)) myrows += 1;

    return myrows;
}

       /* SLIDE: Main Program with OpenMP */
int MLIFE_myrowoffset(int rows, int rank, int nprocs)
{
    int myoffset;

    myoffset = rank * (rows / nprocs);

    if (rank > (rows % nprocs)) myoffset += (rows % nprocs);
    else                        myoffset += rank;

    return myoffset;
}

       /* SLIDE: Main Program with OpenMP */
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
    int myargs[5]; /* array for simple sending of arguments */

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        while ((ret = getopt(argc, argv, "x:y:i:p:r:")) >= 0)
        {
            switch(ret) {
                case 'x':
                    opt_cols = atoi(optarg);
                    break;
                case 'y':
                    opt_rows = atoi(optarg);
                    break;
                case 'i':
                    opt_iter = atoi(optarg);
                    break;
                case 'r':
                    opt_restart_iter = atoi(optarg);
       /* SLIDE: Main Program with OpenMP */
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
    }

    MPI_Bcast(myargs, 5, MPI_INT, 0, MPI_COMM_WORLD);
    opt_rows = myargs[0];
    opt_cols = myargs[1];
    opt_iter = myargs[2];

    MPI_Bcast(opt_prefix, myargs[4], MPI_CHAR, 0, MPI_COMM_WORLD);

    return 0;
}
