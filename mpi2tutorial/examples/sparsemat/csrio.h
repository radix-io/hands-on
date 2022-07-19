/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CSRIO_H
#define CSRIO_H

int CSRIO_Init(MPI_Comm comm, MPI_Info info);
int CSRIO_Finalize(void);
int CSRIO_Write(char *filename, char *title, int n, int my_nz,
		int row_start, int row_end, const int my_ia[],
		const int my_ja[], const double my_a[]);
int CSRIO_Read_rows(char *filename, int n, int nz, int *my_nz_p,
		    int row_start, int row_end, int *my_ia,
		    int **my_ja_p, double **my_a_p);
int CSRIO_Read_header(char *filename, char *title, int *n_p,
		      int *nz_p);

#endif
