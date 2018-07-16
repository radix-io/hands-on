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
    int      num_hosts, mystatus, allstatus, done, numread;
    int      infd, outfd;
    char     outfilename[MAXPATHLEN], controlmsg[CMDSIZE];
    char     buf[BUFSIZE];
    char     soft_limit[20];
    MPI_Info hostinfo;
    MPI_Comm pcpslaves, all_processes;

    MPI_Init( &argc, &argv );

    makehostlist( argv[1], "targets", &num_hosts );
    MPI_Info_create( &hostinfo );
    MPI_Info_set( hostinfo, "file", "targets" );
/* SLIDE: Parallel File Copy */
    sprintf( soft_limit, "0:%d", num_hosts );
    MPI_Info_set( hostinfo, "soft", soft_limit );
    MPI_Comm_spawn( "pcp_slave", MPI_ARGV_NULL, num_hosts, 
                    hostinfo, 0, MPI_COMM_SELF, &pcpslaves,
		    MPI_ERRCODES_IGNORE );
    MPI_Info_free( &hostinfo );
    MPI_Intercomm_merge( pcpslaves, 0, &all_processes );
    strcpy( outfilename, argv[3] );
    if ( (infd = open( argv[2], O_RDONLY ) ) == -1 ) {
        fprintf( stderr, "input %s does not exist\n", argv[2] );
	sprintf( controlmsg, "exit" );
	MPI_Bcast(controlmsg, CMDSIZE, MPI_CHAR, 0,all_processes);
	MPI_Finalize();
	return -1 ;
    }
    else {
        sprintf( controlmsg, "ready" );
        MPI_Bcast(controlmsg, CMDSIZE, MPI_CHAR, 0,all_processes);
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


/* SLIDE: Parallel File Copy */
    if ( allstatus == -1 ) {
        fprintf( stderr, "Output file %s could not be opened\n",
		 outfilename );
	MPI_Finalize();
	return 1 ;
    }
    /* at this point all files have been successfully opened */
    done = 0;
    while (!done) {
        numread = read( infd, buf, BUFSIZE );
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
    MPI_Comm_free( &pcpslaves );
    MPI_Comm_free( &all_processes );
    MPI_Finalize();
    return 0;
}

int makehostlist( char spec[80], char filename[80] )
{

}
