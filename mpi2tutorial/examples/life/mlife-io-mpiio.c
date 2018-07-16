/* SLIDE: MPI-IO Life Checkpoint Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#include "mlife-io.h"

/* MPI-IO implementation of checkpoint and restart for MPI Life
 *
 * Data stored in matrix order, with a header consisting of three
 * integers: matrix size in rows and columns, and iteration no.
 *
 * Each checkpoint is stored in its own file.
 */

static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype);
static int MLIFEIO_Type_create_hdr_rowblk(int **matrix,
                                          int myrows,
                                          int *rows_p,
                                          int *cols_p,
                                          int *iter_p,
                                          MPI_Datatype *newtype);
/* SLIDE: MPI-IO Life Checkpoint Code Walkthrough */
static MPI_Comm mlifeio_comm = MPI_COMM_NULL;

int MLIFEIO_Init(MPI_Comm comm)
{
    int err;

    err = MPI_Comm_dup(comm, &mlifeio_comm);

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

       /* SLIDE: Life MPI-IO Checkpoint/Restart */
int MLIFEIO_Checkpoint(char *prefix, int **matrix, int rows,
                       int cols, int iter, MPI_Info info)
{
    int err;
    int amode = MPI_MODE_WRONLY | MPI_MODE_CREATE |
                MPI_MODE_UNIQUE_OPEN;
    int rank, nprocs;
    int myrows, myoffset;

    MPI_File fh;

    MPI_Datatype type;
    MPI_Offset myfileoffset;

    char filename[64];

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    snprintf(filename, 63, "%s-%d.chkpt", prefix, iter);

    err = MPI_File_open(mlifeio_comm, filename, amode, info, &fh);
    if (err != MPI_SUCCESS) {
        fprintf(stderr, "Error opening %s.\n", filename);
        return err;
    }

    if (rank == 0) {
/* SLIDE: Life MPI-IO Checkpoint/Restart */
        MLIFEIO_Type_create_hdr_rowblk(matrix, myrows, &rows,
                                       &cols, &iter, &type);
        myfileoffset = 0;
    }
    else {
        MLIFEIO_Type_create_rowblk(matrix, myrows, cols, &type);
        myfileoffset = ((myoffset * cols) + 3) * sizeof(int);
    }

    MPI_Type_commit(&type);
    err = MPI_File_write_at_all(fh, myfileoffset, MPI_BOTTOM, 1,
                                    type, MPI_STATUS_IGNORE);
    MPI_Type_free(&type);

    err = MPI_File_close(&fh);
    return err;
}

       /* SLIDE: Life MPI-IO Checkpoint/Restart */
int MLIFEIO_Restart(char *prefix, int **matrix, int rows,
                    int cols, int iter, MPI_Info info)
{
    int err, gErr;
    int amode = MPI_MODE_RDONLY | MPI_MODE_UNIQUE_OPEN;
    int rank, nprocs;
    int myrows, myoffset;
    int buf[3]; /* rows, cols, iteration */

    MPI_File fh;

    MPI_Datatype type;
    MPI_Offset myfileoffset;

    char filename[64];

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    snprintf(filename, 63, "%s-%d.chkpt", prefix, iter);

    err = MPI_File_open(mlifeio_comm, filename, amode, info, &fh);
    if (err != MPI_SUCCESS) return err;

    /* check that rows and cols match */
    err = MPI_File_read_at_all(fh, 0, buf, 3, MPI_INT,
                               MPI_STATUS_IGNORE);

/* SLIDE: Life MPI-IO Checkpoint/Restart */
    /* Have all process check that nothing went wrong */
    MPI_Allreduce(&err, &gErr, 1, MPI_INT, MPI_MAX, mlifeio_comm);
    if (gErr || buf[0] != rows || buf[1] != cols) {
        if (rank == 0) fprintf(stderr, "restart failed.\n");
        return MPI_ERR_OTHER;
    }

    MLIFEIO_Type_create_rowblk(matrix, myrows, cols, &type);
    myfileoffset = ((myoffset * cols) + 3) * sizeof(int);

    MPI_Type_commit(&type);
    err = MPI_File_read_at_all(fh, myfileoffset, MPI_BOTTOM, 1,
                               type, MPI_STATUS_IGNORE);
    MPI_Type_free(&type);

    err = MPI_File_close(&fh);
    return err;
}

       /* SLIDE: Describing Header and Data */
/* MLIFEIO_Type_create_hdr_rowblk
 *
 * Used by process zero to create a type that describes both
 * the header data for a checkpoint and its contribution to
 * the stored matrix.
 *
 * Parameters:
 * matrix  - pointer to the matrix, including boundaries
 * myrows  - number of rows held locally
 * rows_p  - pointer to # of rows in matrix (so we can get its
 *           address for use in the type description)
 * cols_p  - pointer to # of cols in matrix
 * iter_p  - pointer to iteration #
 * newtype - pointer to location to store new type ref.
 */
static int MLIFEIO_Type_create_hdr_rowblk(int **matrix,
                                          int myrows,
                                          int *rows_p,
                                          int *cols_p,
                                          int *iter_p,
                                          MPI_Datatype *newtype)
{
    int err;
    int lens[4] = { 1, 1, 1, 1 };
    MPI_Aint disps[4];
    MPI_Datatype types[4];
    MPI_Datatype rowblk;

    MLIFEIO_Type_create_rowblk(matrix, myrows, *cols_p, &rowblk);
    
    MPI_Get_address(rows_p, &disps[0]);
/* SLIDE: Describing Header and Data */
    MPI_Get_address(cols_p, &disps[1]);
    MPI_Get_address(iter_p, &disps[2]);
    disps[3] = (MPI_Aint) MPI_BOTTOM;
    types[0] = MPI_INT;
    types[1] = MPI_INT;
    types[2] = MPI_INT;
    types[3] = rowblk;

    err = MPI_Type_create_struct(3, lens, disps, types, newtype);

    MPI_Type_free(&rowblk);

    return err;
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
