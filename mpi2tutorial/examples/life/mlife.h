/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MLIFE_H
#define MLIFE_H

/* drand48 should be defined in stdlib.h */
#ifdef NEEDS_DRAND48_DEF
extern void   srand48(void);
extern double drand48(void);
#endif
#ifdef NEEDS_MALLOC_DEF
/* malloc should be defined in stdlib.h */
extern char * malloc(size_t);
extern void free( void * );
#endif

extern char *optarg;

int MLIFE_myrows(int dimsz, int rank, int nprocs);
int MLIFE_myrowoffset(int dimsz, int rank, int nprocs);

int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp, int myrows, 
                        int rows, int cols, int prev, int next);
void MLIFE_exchange_finalize(void);
int MLIFE_exchange(int **matrix, int myrows, int cols);
double life(int rows, int cols, int ntimes, MPI_Comm comm);

#define BORN 1
#define DIES 0

#endif
