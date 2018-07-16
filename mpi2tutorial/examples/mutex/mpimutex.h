/*
 * Copyright (C) 2004 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 */

/* mpimutex implementation */

#include <mpi.h>

typedef struct mpimutex {
    int nprocs, myrank, homerank;
    MPI_Comm comm;
    MPI_Win waitlistwin, pollbytewin;
    MPI_Datatype waitlisttype;
    unsigned char *waitlist; /* only allocated on home rank */
    unsigned char *pollbyte;
} *mpimutex_t;

int MPIMUTEX_Create(int homerank, MPI_Comm comm, mpimutex_t *mutex_p);
int MPIMUTEX_Lock(mpimutex_t mutex);
int MPIMUTEX_Unlock(mpimutex_t mutex);
int MPIMUTEX_Free(mpimutex_t *mutex_p);
