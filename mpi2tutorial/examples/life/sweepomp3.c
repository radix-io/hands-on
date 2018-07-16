/* SLIDE: Implementing the Sweep: Adding Comm/Comp Overlap */
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

#pragma omp parallel default(none) \
 private(k,i,j) firstprivate(ntimes,myrows,rows,cols,opt_prefix) \
    firstprivate(temp,matrix) private(addr)
    for (k = 0; k < ntimes; k++)
    {
#pragma omp single nowait
        MLIFE_exchange(matrix, myrows, cols);

        /* calculate new state for all non-boundary elements and
	   all that do not involve the boundary rows */
#pragma omp for schedule(dynamic)
        for (i = 2; i < myrows; i++) {
            for (j = 1; j < cols+1; j++) {
                temp[i][j] = MLIFE_nextstate(matrix, i, j);
            }
        }

#pragma omp barrier
/* SLIDE: Implementing the Sweep: Adding Comm/Comp Overlap */

	/* We might want this to be an omp single - there is much 
	   less work here */
#pragma omp for 
	for (j = 1; j < cols+1; j++) {
	  temp[1][j] = MLIFE_nextstate(matrix, 1, j);
	  temp[myrows][j] = MLIFE_nextstate(matrix, 1, j );
	}

        /* swap the matrices */
	addr   = matrix;
	matrix = temp;
	temp   = addr;
	
	/* Exploit wait at end of previous omp for */
#pragma omp single private(err) 
	if (0) {
          err = MLIFEIO_Checkpoint(opt_prefix, matrix, rows, cols,
				     k, MPI_INFO_NULL);
	    }
#pragma omp barrier
    }

    /* return the average time taken/processor */
    mytime = MPI_Wtime() - starttime;
    MPI_Reduce(&mytime, &totaltime, 1, MPI_DOUBLE, MPI_SUM, 0,
               comm);

    return totaltime;
}
