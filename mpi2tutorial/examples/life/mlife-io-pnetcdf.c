/* SLIDE: PnetCDF Life Checkpoint Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>
#include <pnetcdf.h>

#include "mlife-io.h"

/* Parallel netCDF implementation of checkpoint and restart for
 * MPI Life
 *
 * Data stored in a 2D variable called "matrix" in matrix order,
 * with dimensions "row" and "col".
 *
 * Each checkpoint is stored in its own file.
 */
static MPI_Comm mlifeio_comm = MPI_COMM_NULL;

static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype);

int MLIFEIO_Init(MPI_Comm comm)
{
    int err;

    err = MPI_Comm_dup(comm, &mlifeio_comm);

/* SLIDE: PnetCDF Life Checkpoint Code Walkthrough */
    return err;
}

int MLIFEIO_Finalize(void)
{
    int err;

    err = MPI_Comm_free(&mlifeio_comm);

    return err;
}

int MLIFEIO_Can_restart(void)
{
    return 1;
}

       /* SLIDE: PnetCDF Life Checkpoint Code Walkthrough */
int MLIFEIO_Checkpoint(char *prefix, int **matrix, int rows,
		       int cols, int iter, MPI_Info info)
{
    int err;
    int cmode = 0;
    int rank, nprocs;
    int myrows, myoffset;

    int ncid, varid, coldim, rowdim, dims[2];
    MPI_Offset start[2];
    MPI_Offset count[2];
    MPI_Datatype type;

    char filename[64];

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    snprintf(filename, 63, "%s-%d.nc", prefix, iter);

    err = ncmpi_create(mlifeio_comm, filename, cmode, info,
		       &ncid);
    if (err != 0) {
	fprintf(stderr, "Error opening %s.\n", filename);
	return MPI_ERR_IO;
    }

/* SLIDE: PnetCDF Life Checkpoint Code Walkthrough */
    ncmpi_def_dim(ncid, "col", cols, &coldim);
    ncmpi_def_dim(ncid, "row", rows, &rowdim);
    dims[0] = coldim;
    dims[1] = rowdim;
    ncmpi_def_var(ncid, "matrix", NC_INT, 2, dims, &varid);

    /* store iteration as global attribute */
    ncmpi_put_att_int(ncid, NC_GLOBAL, "iter", NC_INT, 1, &iter);

    ncmpi_enddef(ncid);

    start[0] = 0; /* col start */
    start[1] = myoffset; /* row start */
    count[0] = cols;
    count[1] = myrows;

    MLIFEIO_Type_create_rowblk(matrix, myrows, cols, &type);
    MPI_Type_commit(&type);

    ncmpi_put_vara_all(ncid, varid, start, count, MPI_BOTTOM, 1,
		       type);

    MPI_Type_free(&type);

    ncmpi_close(ncid);
    return MPI_SUCCESS;
}

       /* SLIDE: PnetCDF Life Checkpoint Code Walkthrough */
int MLIFEIO_Restart(char *prefix, int **matrix, int rows,
		    int cols, int iter, MPI_Info info)
{
    int err = MPI_SUCCESS;
    int rank, nprocs;
    int myrows, myoffset;
    int flag;

    int cmode = 0;
    int ncid, varid, dims[2];
    MPI_Offset start[2];
    MPI_Offset count[2];
    MPI_Offset coldimsz, rowdimsz;
    int i, j, *buf;

    char filename[64];

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    snprintf(filename, 63, "%s-%d.nc", prefix, iter);

    err = ncmpi_open(mlifeio_comm, filename, cmode, info, &ncid);
    if (err != 0) {
	fprintf(stderr, "Error opening %s.\n", filename);
	return MPI_ERR_IO;
    }
/* SLIDE: Discovering Variable Dimensions */
    err = ncmpi_inq_varid(ncid, "matrix", &varid);
    if (err != 0) {
	return MPI_ERR_IO;
    }

    /* verify that dimensions in file are same as input row/col */
    err = ncmpi_inq_vardimid(ncid, varid, dims);
    if (err != 0) {
	return MPI_ERR_IO;
    }

    err = ncmpi_inq_dimlen(ncid, dims[0], &coldimsz);
    if (coldimsz != cols) {
	fprintf(stderr, "cols does not match\n");
	return MPI_ERR_IO;
    }

    err = ncmpi_inq_dimlen(ncid, dims[1], &rowdimsz);
    if (rowdimsz != rows) {
	fprintf(stderr, "rows does not match\n");
	return MPI_ERR_IO;
    }

	/* SLIDE: Discovering Variable Dimensions */
    buf = (int *) malloc(myrows * cols * sizeof(int));
    flag = (buf == NULL);
    /* See if any process failed to allocate memory */
    MPI_Allreduce(MPI_IN_PLACE, &flag, 1, MPI_INT, MPI_LOR, 
		  mlifeio_comm);
    if (flag) {
	return MPI_ERR_IO;
    }

    start[0] = 0; /* col start */
    start[1] = myoffset; /* row start */
    count[0] = cols;
    count[1] = myrows;
    ncmpi_get_vara_int_all(ncid, varid, start, count, buf);

    for (i=0; i < myrows; i++) {
	for (j=0; j < cols; j++) {
	    matrix[i+1][j] = buf[(i*cols) + j];
	}
    }

    free(buf);

    return MPI_SUCCESS;
}
       /* SLIDE: Placing Data in Checkpoint */
/* MLIFEIO_Type_create_rowblk
 *
 * See stdio version for details (this is a copy).
 */
static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype)
{
    int err, len;
    MPI_Datatype vectype;

    MPI_Aint disp;

    /* since our data is in one block, access is very regular */
    err = MPI_Type_vector(myrows, cols, cols+2, MPI_INTEGER,
                          &vectype);
    if (err != MPI_SUCCESS) return err;

    /* wrap the vector in a type starting at the right offset */
    len = 1;
    MPI_Get_address(&matrix[1][1], &disp);
    err = MPI_Type_create_hindexed(1,&len,&disp,vectype,newtype);

    MPI_Type_free(&vectype); /* decrement reference count */

    return err;
}
