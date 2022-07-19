/* SLIDE: stdio CSRIO Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

/* 
 * Our storage format for CSR will use native byte format.
 * The file will contain:
 *
 * title (char, 80 bytes, fixed size)
 * n (int)
 * nz (int)
 * ia[i], i=1,...,n+1 (int array)
 * ja[i], i=1,...,nz  (int array)
 * a[i],  i=1,...,nz  (double array)
 *
 * See README.txt for more information on CSR.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "csrio.h"

static MPI_Comm csrio_comm = MPI_COMM_NULL;

       /* SLIDE: stdio CSRIO Code Walkthrough */
/* CSRIO_Init
 *
 * Parameters:
 * comm - communicator describing group of processes that
 *        will perform I/O
 * info - set of hints passed to CSRIO calls (ignored)
 */
int CSRIO_Init(MPI_Comm comm, MPI_Info info)
{
    int err;

    err = MPI_Comm_dup(comm, &csrio_comm);

    return err;
}

int CSRIO_Finalize(void)
{
    MPI_Comm_free(&csrio_comm);

    return MPI_SUCCESS;
}

       /* SLIDE: Writing Sparse Matrices (stdout) */
/* CSRIO_Write
 *
 * Parameters:
 * filename  - name of file to hold data
 * n         - number of rows in matrix
 * my_nz     - number of nonzero values to read into local buffer
 * row_start - first row to write
 * row_end   - last row to write
 * my_ia     - local row start indices
 * my_ja     - column indices for local array values
 * my_a      - data values
 * 
 * Returns MPI_SUCCESS on success, MPI error code on error.
 */
int CSRIO_Write(char *filename, char *title, int n, int my_nz,
		int row_start, int row_end, const int my_ia[],
		const int my_ja[], const double my_a[])
{
    int i, err;
    int *tmp_ia, my_rows;
    int prev_nz, tot_nz;

    int rank, nprocs;

    MPI_Comm_size(csrio_comm, &nprocs);
    MPI_Comm_rank(csrio_comm, &rank);

    my_rows = row_end - row_start + 1;
    
    err = MPI_Exscan(&my_nz, &prev_nz, 1, MPI_INT, MPI_SUM,
		     csrio_comm);
/* SLIDE: Writing Sparse Matrices (stdout) */
    err = MPI_Allreduce(&my_nz, &tot_nz, 1, MPI_INT, MPI_SUM,
			csrio_comm);

     /* copy ia; adjust to be relative to global data */
    tmp_ia = (int *) malloc(my_rows * sizeof(int));
    if (tmp_ia == NULL) return MPI_ERR_IO;

    for (i=0; i < row_end - row_start + 1; i++) {
        tmp_ia[i] = my_ia[i] + prev_nz;
    }

    if (rank != 0) {
        /* gather count of rows to rank 0, then gather local ia */
        err = MPI_Gather(&my_rows, 1, MPI_INT, NULL, 1, MPI_INT,
                         0, csrio_comm);
        err = MPI_Gatherv(tmp_ia, my_rows, MPI_INT, NULL, NULL,
                          NULL, MPI_INT, 0, csrio_comm);

        /* gather count of nonzeros at rank 0, then gather ja */
        err = MPI_Gather(&my_nz, 1, MPI_INT, NULL, 1, MPI_INT,
                         0, csrio_comm);
        err = MPI_Gatherv((void *) my_ja, my_nz, MPI_INT, NULL,
                          NULL, NULL, MPI_INT, 0, csrio_comm);

        /* gather a using count of nonzeros from last step */
        err = MPI_Gatherv((void *) my_a, my_nz, MPI_DOUBLE, NULL,
                          NULL, NULL, MPI_DOUBLE, 0, csrio_comm);
    }
    else /* rank 0 */ {
        int *all_ia, *all_ja, *proc_n, *n_disp;
	int *proc_nz, *nz_disp;
/* SLIDE: Writing Sparse Matrices (stdout) */
        double *all_a;

        printf("# filename = %s\ntitle = %s\n", filename, title);
        printf("n = %d\nnz = %d\n", n, tot_nz);

        /* gather count of rows */
        all_ia = (int *) malloc(n * sizeof(int));
        proc_n = (int *) malloc(nprocs * sizeof(int));
        n_disp = (int *) malloc(nprocs * sizeof(int));
        err = MPI_Gather(&my_rows, 1, MPI_INT, proc_n, 1, MPI_INT,
                         0, csrio_comm);

        /* compute displacements for received ia values */
        n_disp[0] = 0;
        for (i=1; i < nprocs; i++) {
            n_disp[i] = n_disp[i-1] + proc_n[i-1];
        }

        /* gather ia */
        err = MPI_Gatherv(tmp_ia, my_rows, MPI_INT, all_ia,
			  proc_n, n_disp, MPI_INT, 0, csrio_comm);

        /* print ia */
        printf("ia[0..%d] = ( ", n-1);
        for (i=0; i < n; i++) {
            printf("%d%s", all_ia[i], (i < n-1) ? " " : " )\n");
        }
        free(all_ia);
        free(proc_n);
        free(n_disp);

/* SLIDE: Writing Sparse Matrices (stdout) */
        /* gather count of nonzeros */
        all_ja     = (int *) malloc(tot_nz * sizeof(int));
        proc_nz    = (int *) malloc(nprocs * sizeof(int));
        nz_disp    = (int *) malloc(nprocs * sizeof(int));
        err = MPI_Gather(&my_nz, 1, MPI_INT, proc_nz, 1, MPI_INT,
                         0, csrio_comm);

        /* compute displacements for received ja values */
        nz_disp[0] = 0;
        for (i=1; i < nprocs; i++) {
            nz_disp[i] = nz_disp[i-1] + proc_nz[i-1];
        }

        /* gather ja */
        err = MPI_Gatherv((void *) my_ja, my_nz, MPI_INT, all_ja,
                          proc_nz, nz_disp, MPI_INT, 0,
			  csrio_comm);

        /* print ja */
        printf("ja[0..%d] = ( ", tot_nz-1);
        for (i=0; i < tot_nz; i++) {
            printf("%d%s", all_ja[i],
		   (i < tot_nz-1) ? " " : " )\n");
        }
        free(all_ia);

        /* gather a using count of nonzeros from last step */
        all_a = (double *) malloc(tot_nz * sizeof(double));
        err = MPI_Gatherv((void *) my_a, my_nz, MPI_DOUBLE, all_a,
                          proc_nz, nz_disp, MPI_DOUBLE, 0,
			  csrio_comm);

/* SLIDE: Writing Sparse Matrices (stdout) */
        /* print a */
        printf("a[0..%d]  = ( ", tot_nz-1);
        for (i=0; i < tot_nz; i++) {
            printf("%.3lf%s", all_a[i],
		   (i < tot_nz-1) ? " " : " )\n");
        }
        free(all_a);
        free(proc_nz);
        free(nz_disp);
    }

    free(tmp_ia);

    return err;
}

       /* SLIDE: Writing Sparse Matrices (stdout) */
int CSRIO_Read_header(char *filename, char *title, int *n_p,
		      int *nz_p)
{
    return MPI_ERR_IO;
}

int CSRIO_Read_rows(char *filename, int n, int nz, int *my_nz_p,
                    int row_start, int row_end, int *my_ia,
                    int **my_ja_p, double **my_a_p)
{
    return MPI_ERR_IO;
}

