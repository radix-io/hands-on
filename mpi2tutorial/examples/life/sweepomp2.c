/* SLIDE: Implementing the Sweep: The Simple Version */
#include "mpi.h"
#include <omp.h>

double MLIFE_Sweep( int **matrix, int **temp, 
		    int myrows, int rows, int cols, int ntimes, 
		    char opt_prefix[64], MPI_Comm comm )
{
    int i, j, k, err;
    int **addr;
    double mytime, totaltime=0.0, starttime;

    starttime = MPI_Wtime();

    for (k = 0; k < ntimes; k++)
    {
        MLIFE_exchange(matrix, myrows, cols);

        /* calculate new state for all non-boundary elements */
#pragma omp parallel for default(none) private(i,j) \
  firstprivate(myrows,cols)			    \
  shared(temp,matrix)
        for (i = 1; i <= myrows; i++) {
            for (j = 1; j < cols+1; j++) {
                temp[i][j] = MLIFE_nextstate(matrix, i, j);
            }
        }

       /* SLIDE: Implementing the Sweep: The Simple Version */
        /* swap the matrices */
	addr   = matrix;
	matrix = temp;
	temp   = addr;
	
	/* Exploit wait at end of previous omp for */
	if (0) {
	 err = MLIFEIO_Checkpoint(opt_prefix, matrix, rows, cols, 
        			     k, MPI_INFO_NULL);
	    }
    }

    /* return the average time taken/processor */
    mytime = MPI_Wtime() - starttime;
    MPI_Reduce(&mytime, &totaltime, 1, MPI_DOUBLE, MPI_SUM, 0,
               comm);

    return totaltime;
}
