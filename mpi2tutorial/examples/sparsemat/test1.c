/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>

#include "csrio.h"

int main(int argc, char *argv[])
{
    int i, err, my_n = 10, my_nz = 10;
    int my_ia[11];
    int my_ja[10];
    double my_a[10];
    int rank, nprocs;

    MPI_Init(&argc, &argv);
    CSRIO_Init(MPI_COMM_WORLD, MPI_INFO_NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* initialize local variables */
    for (i=0; i < my_n; i++) {
	my_ia[i] = i;
    }

    for (i=0; i < my_nz; i++) {
	my_ja[i] = i;
	my_a[i]  = (double) i;
    }

    /* checkpoint */
    err = CSRIO_Write("checkpoint.out", "test1", my_n * nprocs, my_nz,
		      nprocs * my_n, (nprocs + 1) * my_n - 1, my_ia,
		      my_ja, my_a);

    CSRIO_Finalize();
    MPI_Finalize();

    return 0;
}
