/* SLIDE: Parallel File Copy */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE    256*1024
#define CMDSIZE    80
#define MAXPATHLEN 256

int main( int argc, char *argv[] )
{
    int      mystatus, allstatus, done, numread;
    char     outfilename[MAXPATHLEN], controlmsg[CMDSIZE];
    int      outfd;
    char     buf[BUFSIZE];
    MPI_Comm slavecomm, all_processes;

    MPI_Init( &argc, &argv );

    MPI_Comm_get_parent( &slavecomm );
    MPI_Intercomm_merge( slavecomm, 1, &all_processes );
    MPI_Bcast( controlmsg, CMDSIZE, MPI_CHAR, 0, 
               all_processes );
    if ( strcmp( controlmsg, "exit" ) == 0 ) {
/* SLIDE: Parallel File Copy */
        MPI_Finalize();
	return 1;
    }

    MPI_Bcast( outfilename, MAXPATHLEN, MPI_CHAR, 0, 
               all_processes );
    if ( (outfd = open( outfilename, O_CREAT|O_WRONLY|O_TRUNC,
			S_IRWXU ) ) == -1 ) 
        mystatus = -1;
    else
        mystatus = 0;
    MPI_Allreduce( &mystatus, &allstatus, 1, MPI_INT, MPI_MIN,
		   all_processes );
    if ( allstatus == -1 ) {
	MPI_Finalize();
	return -1;
    }
    /* at this point all files have been successfully opened */
    
    done = 0;
    while ( !done ) {
	MPI_Bcast( &numread, 1, MPI_INT, 0, all_processes );
	if ( numread > 0 ) {
	    MPI_Bcast( buf, numread, MPI_BYTE, 0, all_processes );
	    write( outfd, buf, numread ); 
	}
	else {	  
	    close( outfd );
	    done = 1;
	}
    }
/* SLIDE: Parallel File Copy */
    MPI_Comm_free( &slavecomm );
    MPI_Comm_free( &all_processes );
    MPI_Finalize();
    return 0;
}
