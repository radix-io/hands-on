/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MLIFE_IO_H
#define MLIFE_IO_H

int MLIFEIO_Init(MPI_Comm comm);
int MLIFEIO_Can_restart(void);
int MLIFEIO_Info_set(MPI_Info info);
int MLIFEIO_Restart(char *prefix, int **matrix, int rows, int cols, int iter,
		    MPI_Info info);
int MLIFEIO_Checkpoint(char *prefix, int **matrix, int rows, int cols, 
		       int iter, MPI_Info info);
int MLIFEIO_Finalize(void);

extern int MLIFE_myrows(int dimsz, int rank, int nprocs);
extern int MLIFE_myrowoffset(int dimsz, int rank, int nprocs);

#endif
