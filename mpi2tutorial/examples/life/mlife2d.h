/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MLIFE_H
#define MLIFE_H

#if 0
extern void   srand48();
extern double drand48();
extern char * malloc();
#endif

extern char *optarg;

int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp, int rows, 
                        int cols, int LRows, int LCols, int above, int below, 
                        int left, int right);
void MLIFE_exchange_finalize(void);
int MLIFE_exchange(int **matrix, int myrows, int cols);

void MLIFE_MeshDecomp(int rank, int nprocs, 
		      int GRows, int GCols, 
		      int *leftP, int *rightP, int *topP, int *bottomP, 
		      int *LRowsP, int *LColsP, 
		      int *GFirstRowP, int *GFirstColP);


double life(int rows, int cols, int ntimes, MPI_Comm comm);

#define BORN 1
#define DIES 0

#endif
