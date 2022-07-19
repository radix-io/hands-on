/* SLIDE: MPI-IO CSRIO Code Walkthrough */
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
static MPI_Info csrio_info = MPI_INFO_NULL;

       /* SLIDE: MPI-IO CSRIO Code Walkthrough */
/* CSRIO_Init
 *
 * Parameters:
 * comm - communicator describing group of processes that will
 *        perform I/O
 * info - set of hints passed to CSRIO calls
 */
int CSRIO_Init(MPI_Comm comm, MPI_Info info)
{
    int err;

    err = MPI_Comm_dup(comm, &csrio_comm);
    if (err == MPI_SUCCESS && info != MPI_INFO_NULL) {
	err = MPI_Info_dup(info, &csrio_info);
    }

    return err;
}

int CSRIO_Finalize(void)
{
    MPI_Comm_free(&csrio_comm);
    if (csrio_info != MPI_INFO_NULL) {
        MPI_Info_free(&csrio_info);
    }

    return MPI_SUCCESS;
}

       /* SLIDE: Reading Sparse Matrix Header */
/* CSRIO_Read_header
 *
 * Parameters:
 * filename - name of file from which header will be read
 * title    - pointer to buffer of at least 80 characters that
 *            will hold title from file if call completes
 *            successfully
 * n_p      - address of integer in which number of rows is
 *            stored on success
 * nz_p     - address of integer in which number of nonzeros is
 *            stored on success
 *
 * Returns MPI_SUCCESS on success, MPI error code on error.
 */
int CSRIO_Read_header(char *filename, char *title, int *n_p,
		      int *nz_p)
{
    int err = 0, ioerr;
    int nrnz[2];

    int rank;
    int amode = MPI_MODE_RDONLY | MPI_MODE_UNIQUE_OPEN;

    MPI_Datatype type;
    int lens[3];
    MPI_Aint disps[3];
    MPI_Datatype types[3];

    MPI_Comm_rank(csrio_comm, &rank);


/* SLIDE: Reading Sparse Matrix Header */
    /* often it is faster for one process to open/access/close
     * the file when only a small amount of data is going to be
     * accessed.
     */
    if (rank == 0) {
        MPI_File fh;
        MPI_Status status;

        err = MPI_File_open(MPI_COMM_SELF, filename, amode,
			    csrio_info, &fh);
        if (err == MPI_SUCCESS) {
            err = MPI_File_read_at(fh, 0, title, 80, MPI_CHAR,
				   &status);
        }
        if (err == MPI_SUCCESS) {
            err = MPI_File_read_at(fh, 80, nrnz, 2, MPI_INT,
				   &status);
        }

        MPI_File_close(&fh);
    }
    ioerr = err;
    
    /* define a struct that describes all our data */
    lens[0] = 80;
    lens[1] = 2;
    lens[2] = 1;
    MPI_Get_address(title, &disps[0]);
    MPI_Get_address(nrnz, &disps[1]);
    MPI_Get_address(&ioerr, &disps[2]);
    types[0] = MPI_CHAR;
/* SLIDE: Reading Sparse Matrix Header */
    types[1] = MPI_INT;
    types[2] = MPI_INT;

    MPI_Type_create_struct(3, lens, disps, types, &type);

    /* broadcast the header data to everyone */
    err = MPI_Bcast(MPI_BOTTOM, 1, type, 0, csrio_comm);
    if (err == MPI_SUCCESS && ioerr == MPI_SUCCESS) {
        *n_p = nrnz[0];
        *nz_p = nrnz[1];

        return MPI_SUCCESS;
    }
    else {
        return (err != MPI_SUCCESS) ? err : ioerr;
    }
}

       /* SLIDE: Reading Sparse Matrix Data */
/* CSRIO_Read_rows
 *
 * Parameters:
 * n         - number of rows in matrix
 * nz        - number of nonzero values in matrix
 * my_nz     - maximum number of nonzero values to read into
 *             local buffer (ignored if equal to 0); holds actual
 *             number of nonzero values on success
 * row_start - first row to read (0-origin)
 * row_end   - last row to read
 * my_ia     - pointer to local memory for storing row start
 *             indices
 * my_ja_p   - address of pointer to local memory for storing
 *             column indices (if not NULL, then region must be
 *             large enough for my_nz values)
 * my_a_p    - address of pointer to local memory for storing
 *             data values (if not NULL, then region must be
 *             large enough for my_nz values)
 *
 * Notes:
 * If (*my_ja_p == NULL) then memory will be allocated.  Likewise,
 * if (*my_a_p == NULL) then memory will be allocated.
 *
 * Returns MPI_SUCCESS on success, MPI error code on error.
 */
int CSRIO_Read_rows(char *filename, int n, int nz, int *my_nz_p,
		    int row_start, int row_end, int *my_ia,
		    int **my_ja_p, double **my_a_p)
{
    int i, count, err, lens[2], my_mem_ok, all_mem_ok;
    int next_row_ia, my_nz_ok, my_ja_ok = 1, my_a_ok = 1;
/* SLIDE: Reading Sparse Matrix Data */
    MPI_Aint my_ia_off, my_ja_off, my_a_off, disps[2];

    int amode = MPI_MODE_RDONLY | MPI_MODE_UNIQUE_OPEN;
    MPI_File fh;
    MPI_Status status;
    MPI_Datatype type;

    err = MPI_File_open(csrio_comm, filename, amode, csrio_info,
			&fh);
    if (err != MPI_SUCCESS) {
        return err;
    }

    my_ia_off = 80 * sizeof(char) + (2 + row_start) * sizeof(int);

    /* must read one additional row start to calculate
     * no. of elements
     */
    lens[0] = row_end - row_start + 1;
    lens[1] = 1;
    MPI_Get_address(my_ia, &disps[0]);
    MPI_Get_address(&next_row_ia, &disps[1]);

    MPI_Type_hindexed(2, lens, disps, MPI_INT, &type);
    MPI_Type_commit(&type);

    err = MPI_File_read_at_all(fh, my_ia_off, MPI_BOTTOM, 1, type,
			       &status);
    if (err != MPI_SUCCESS) {
        return err;
    }
/* SLIDE: Reading Sparse Matrix Data */
    MPI_Type_free(&type);

    count = next_row_ia - my_ia[0];

    /* verify local nz value, allocate memory as necessary */
    my_nz_ok = (*my_nz_p == 0 || *my_nz_p >= count) ? 1 : 0;

    if (*my_ja_p == NULL) {
        *my_ja_p = (int *) malloc(count * sizeof(int));
        if (*my_ja_p == NULL) my_ja_ok = 0;
    }

    if (*my_a_p == NULL) {
        *my_a_p = (double *) malloc(count * sizeof(double));
        if (*my_a_p == NULL) my_a_ok = 0;
    }

    /* verify memory regions and abort now on error */
    my_mem_ok = (my_nz_ok && my_a_ok && my_ja_ok) ? 1 : 0;

    MPI_Allreduce(&my_mem_ok, &all_mem_ok, 1, MPI_INT, MPI_MIN,
		  csrio_comm);
    if (!all_mem_ok) {
        return MPI_ERR_IO;
    }

    /* save actual number of local nonzeros */
    *my_nz_p = count;

    /* read local portion of ja */
    my_ja_off = 80 * sizeof(char) + (2+n+my_ia[0]) * sizeof(int);
/* SLIDE: Reading Sparse Matrix Data */
    err = MPI_File_read_at_all(fh, my_ja_off, *my_ja_p, count,
                               MPI_INT, &status);
    if (err != MPI_SUCCESS) {
        return err;
    }

    /* read local portion of a */
    my_a_off = 80 * sizeof(char) + (2 + n + nz) * sizeof(int) +
        my_ia[0] * sizeof(double);

    err = MPI_File_read_at_all(fh, my_a_off, *my_a_p, count,
                               MPI_DOUBLE, &status);
    if (err != MPI_SUCCESS) {
        return err;
    }

    /* convert ia values to local references */
    for (i=1; i < row_end - row_start + 1; i++) {
        my_ia[i] -= my_ia[0];
    }
    my_ia[0] = 0;

    return MPI_SUCCESS;
}

       /* SLIDE: Writing Sparse Matrices */
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
    int *tmp_ia;

    int prev_nz, tot_nz;

    int amode = MPI_MODE_WRONLY | MPI_MODE_CREATE |
        MPI_MODE_UNIQUE_OPEN;
    int rank, nprocs;

    MPI_File fh;
    MPI_Status status;

    MPI_Offset myfilerowoffset, myfilecoloffset, myfiledataoffset;
/* SLIDE: Writing Sparse Matrices */
    MPI_Datatype memtype, filetype;
    int blklens[3];
    MPI_Aint disps[3];
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_DOUBLE};

    MPI_Comm_size(csrio_comm, &nprocs);
    MPI_Comm_rank(csrio_comm, &rank);
    
    err = MPI_Exscan(&my_nz, &prev_nz, 1, MPI_INT, MPI_SUM,
		     csrio_comm);

    err = MPI_Allreduce(&my_nz, &tot_nz, 1, MPI_INT, MPI_SUM,
			csrio_comm);

    printf("rank %d has %d elements, will start at %d\n", rank,
	   my_nz, prev_nz);

    err = MPI_File_open(csrio_comm, filename, amode, csrio_info,
			&fh);
    if (err != MPI_SUCCESS) return err;

    /* rank 0 writes title, # of rows, and count of nonzeros */
    if (rank == 0) {
        char titlebuf[80];
        int intbuf[2];

        memset(titlebuf, 0, 80);
        strncpy(titlebuf, title, 79);
        err = MPI_File_write_at(fh, 0, titlebuf, 80, MPI_CHAR,
				&status);

/* SLIDE: Writing Sparse Matrices */
        intbuf[0] = n;
        intbuf[1] = tot_nz;
        err = MPI_File_write_at(fh, 80, intbuf, 2, MPI_INT,
				&status);
    }

    /* copy ia; adjust to be relative to global data */
    tmp_ia = (int *) malloc((row_end - row_start + 1) *
			    sizeof(int));
    if (tmp_ia == NULL) return MPI_ERR_IO;

    for (i=0; i < row_end - row_start + 1; i++) {
        tmp_ia[i] = my_ia[i] + prev_nz;
    }

    /* set block lengths (same for both types) */
    blklens[0] = row_end - row_start + 1;
    blklens[1] = my_nz;
    blklens[2] = my_nz;

    /* calculate file offsets and create file type */
    myfilerowoffset = 80 * sizeof(char) +
	(2+row_start) * sizeof(int);

    myfilecoloffset = 80 * sizeof(char) +
	(2+n+prev_nz) * sizeof(int);
        
    myfiledataoffset = 80 * sizeof(char) + (2+n+tot_nz) *
	sizeof(int) + prev_nz * sizeof(double);

    disps[0] = myfilerowoffset;
/* SLIDE: Writing Sparse Matrices */
    disps[1] = myfilecoloffset;
    disps[2] = myfiledataoffset;

    err = MPI_Type_create_struct(3, blklens, disps, types,
				 &filetype);
    MPI_Type_commit(&filetype);

    /* create memory type */
    MPI_Get_address(tmp_ia, &disps[0]);
    MPI_Get_address((void *)&my_ja[0],  &disps[1]);
    MPI_Get_address((void *)&my_a[0],   &disps[2]);

    err = MPI_Type_create_struct(3, blklens, disps, types,
				 &memtype);
    MPI_Type_commit(&memtype);

    /* set file view */
    err = MPI_File_set_view(fh, 0, MPI_BYTE, filetype, "native",
			    csrio_info);

    /* everyone writes their own row offsets, columns, and 
     * data with one big noncontiguous write (in memory and 
     * file)
     */
    err = MPI_File_write_all(fh, MPI_BOTTOM, 1, memtype, &status);

    free(tmp_ia);
    MPI_Type_free(&filetype);
    MPI_Type_free(&memtype);
    return err;
}
