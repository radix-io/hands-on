/* SLIDE: Mutex Code Walkthrough */
/*
 * Copyright (C) 2004 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 */

/* mpimutex implementation using hybrid algorithm */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mpimutex.h"

#define WAKEUP 13

int MPIMUTEX_Create(int homerank, MPI_Comm comm,
                    mpimutex_t *mutex_p)
{
    int nprocs, myrank, mpi_err;
    mpimutex_t mutex = NULL;

    int blklens[2];
    MPI_Aint disps[2];

       /* SLIDE: Mutex Code Walkthrough */
    /* allocate mpimutex structure, fill in */
    mutex = malloc(sizeof(struct mpimutex));
    if (!mutex) goto err_return;
    mutex->homerank = homerank;
    mutex->waitlistwin = MPI_WIN_NULL;
    mutex->waitlisttype = MPI_DATATYPE_NULL;
    mutex->waitlist = NULL;
    mutex->comm = MPI_COMM_NULL;
    
    MPI_Comm_rank(comm, &myrank);
    MPI_Comm_size(comm, &nprocs);
    mutex->myrank = myrank;
    mutex->nprocs = nprocs;

    blklens[0] = mutex->myrank;
    disps[0]   = 0;
    blklens[1] = mutex->nprocs - mutex->myrank - 1;
    disps[1]   = mutex->myrank + 1;

    /* build type once */
    mpi_err = MPI_Type_indexed(2, blklens, disps, MPI_BYTE,
			       &mutex->waitlisttype);
    if (mpi_err != MPI_SUCCESS) goto err_return;
    mpi_err = MPI_Type_commit(&mutex->waitlisttype);
    if (mpi_err != MPI_SUCCESS) goto err_return;

    MPI_Comm_dup(comm, &mutex->comm);
    if (mpi_err != MPI_SUCCESS) goto err_return;
       /* SLIDE: Mutex Code Walkthrough */
    if (myrank == homerank) {
	/* allocate waitlist on this proc */
	mpi_err = MPI_Alloc_mem(nprocs, MPI_INFO_NULL,
                                &mutex->waitlist);
	if (mpi_err != MPI_SUCCESS) goto err_return;
	memset(mutex->waitlist, 0, nprocs);

	mpi_err = MPI_Win_create(mutex->waitlist, nprocs, 1,
                                 MPI_INFO_NULL, mutex->comm,
                                 &mutex->waitlistwin);
    }
    else {
        /* this process contributes nothing */
	mpi_err = MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL,
				 mutex->comm,
                                 &mutex->waitlistwin);
    }
    if (mpi_err != MPI_SUCCESS) goto err_return;

    *mutex_p = mutex;
    return MPI_SUCCESS;
       /* SLIDE: Mutex Code Walkthrough */
 err_return:
    if (mutex) {
        /* clean up all the structures we allocated */
	if (mutex->waitlistwin != MPI_WIN_NULL)
	    MPI_Win_free(&mutex->waitlistwin);
	if (mutex->waitlist)
	    MPI_Free_mem(mutex->waitlist);
	if (mutex->waitlisttype != MPI_DATATYPE_NULL)
	    MPI_Type_free(&mutex->waitlisttype);
	if (mutex->comm != MPI_COMM_NULL)
	    MPI_Comm_free(&mutex->comm);
	free(mutex);
    }
    return MPI_ERR_UNKNOWN;
}

       /* SLIDE: Mutex Code Walkthrough */
int MPIMUTEX_Lock(mpimutex_t mutex)
{
    int mpi_err, i;
    unsigned char val = 1;
    unsigned char *waitlistcopy = NULL;

    waitlistcopy = malloc(mutex->nprocs-1);
    if (!waitlistcopy) goto err_return;

    /* add self to waitlist */
    mpi_err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mutex->homerank, 0,
			   mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;

    mpi_err = MPI_Get(waitlistcopy, mutex->nprocs-1, MPI_BYTE,
		      mutex->homerank, 0, 1, mutex->waitlisttype,
		      mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;
    mpi_err = MPI_Put(&val, 1, MPI_BYTE,
		      mutex->homerank, mutex->myrank, 1, MPI_BYTE,
		      mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;

    mpi_err = MPI_Win_unlock(mutex->homerank, mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;

       /* SLIDE: Mutex Code Walkthrough */
    /* check to see if lock is already held */
    for (i=0; i < (mutex->nprocs-1) && waitlistcopy[i] == 0; i++);

    if (i < mutex->nprocs - 1) {
	/* wait for notification from some other process */
	mpi_err = MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE,
                           WAKEUP, mutex->comm,
                           MPI_STATUS_IGNORE);
	if (mpi_err != MPI_SUCCESS) goto err_return;
    }

    free(waitlistcopy);

    return MPI_SUCCESS;

 err_return:
    printf("error!\n");
    if (waitlistcopy) free(waitlistcopy);

    return MPI_ERR_UNKNOWN;
}

       /* SLIDE: Mutex Code Walkthrough */
int MPIMUTEX_Unlock(mpimutex_t mutex)
{
    int mpi_err, i;
    unsigned char val = 0;
    unsigned char *waitlistcopy;

    /* TODO: allocate waitlistcopy only once */
    waitlistcopy = malloc(mutex->nprocs-1);
    if (!waitlistcopy) goto err_return;

    /* remove self from waitlist */
    mpi_err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mutex->homerank, 0,
			   mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;

    mpi_err = MPI_Get(waitlistcopy, mutex->nprocs-1, MPI_BYTE,
		      mutex->homerank, 0, 1, mutex->waitlisttype,
		      mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;
    mpi_err = MPI_Put(&val, 1, MPI_BYTE,
		      mutex->homerank, mutex->myrank, 1, MPI_BYTE,
		      mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;

    mpi_err = MPI_Win_unlock(mutex->homerank, mutex->waitlistwin);
    if (mpi_err != MPI_SUCCESS) goto err_return;
       /* SLIDE: Mutex Code Walkthrough */
    /* check to see if someone is waiting for the lock */
    for (i=0; i < (mutex->nprocs-1) && waitlistcopy[i] == 0; i++);

    if (i < mutex->nprocs - 1) {
	int nextrank;

	/* find next rank waiting for the lock.  we start with
	 * rank after ours and look in order to ensure fairness.
	 */
	nextrank = mutex->myrank;
	while (nextrank < (mutex->nprocs - 1) &&
               waitlistcopy[nextrank] == 0)
	{
	    nextrank++;
	}
	if (nextrank < mutex->nprocs - 1) {
	    /* nextrank is off by one from skipping local byte */
	    nextrank++;
	}
	else {
	    nextrank = 0;
	    while (nextrank < mutex->myrank &&
                   waitlistcopy[nextrank] == 0)
	    {
		nextrank++;
	    }

	    assert(nextrank != mutex->myrank);
	}


	/* notify next rank */
	mpi_err = MPI_Send(NULL, 0, MPI_BYTE, nextrank, WAKEUP,
			   mutex->comm);
	if (mpi_err != MPI_SUCCESS) goto err_return;
    }

    free(waitlistcopy);

    return MPI_SUCCESS;

 err_return:
    printf("error!\n");
    if (waitlistcopy) free(waitlistcopy);
    return MPI_ERR_UNKNOWN;
}
       /* SLIDE: Mutex Code Walkthrough */
int MPIMUTEX_Free(mpimutex_t *mutex_p)
{
    mpimutex_t mutex = *mutex_p;

    if (mutex) {
	if (mutex->waitlistwin != MPI_WIN_NULL) {
	    MPI_Win_free(&mutex->waitlistwin);
	}

	if (mutex->waitlist) {
	    MPI_Free_mem(mutex->waitlist);
	}

	if (mutex->waitlisttype != MPI_DATATYPE_NULL) {
	    MPI_Type_free(&mutex->waitlisttype);
	}

	if (mutex->comm != MPI_COMM_NULL) {
	    MPI_Comm_free(&mutex->comm);
	}

	free(mutex);
    }

    *mutex_p = NULL;

    return MPI_SUCCESS;
}
