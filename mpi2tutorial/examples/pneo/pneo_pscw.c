/* SLIDE: pNeo Code Walkthrough */
/*  This is an abstraction of the pNeo brain simulation program,
    written to illustrate the MPI one-sided operations.

    Run with

      pneo <filename>

    where <filename> is a file of connections, one on each line
    consisting of a blank-separated pair of integers in character
    format. Only one cell per process is modelled in this version.

    This is the post-start-complete-wait version.  (pscw)

*/

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

void init_state( char * );
void reset_state( void );
int  ready_to_fire( void );
void setup_groups( void );
void compute_state( void );
void output_spikes( void );
void dump_local_arrays( void );
void dump_window( void );

       /* SLIDE: pNeo Code Walkthrough */
/* Connections to other cells are prepresented by an array of
 * inputs (inconnections) and an array of outputs (outconnections)
 * A connection array entry contains the rank of the other process
 * and the values of the incoming or outgoing spikes.  The input
 * connection arrays are the windows. The outconnections also
 * contain the displacement into the destination window of the
 * connection.
 */

int *inarray;		    /* array of possible input spikes */
int *inranks;               /* array of possible sources */
int inarray_count;

MPI_Win win;			/* the window for this process,
				 * which will be identified with
				 * the inarray */
typedef struct {
    int dest;
    int disp;
} outconnection;

outconnection *outarray;	/* array of outputs */
int outarray_count;

int state;			/* state of the cell */

/* For post-start-complete-wait version only */
int *outranks;
MPI_Group ingroup, outgroup;

int numprocs, myrank;
/* SLIDE: pNeo Code Walkthrough */
int itercount;
int max_steps = 100;		/* number of steps to run */
int main(int argc, char *argv[])
{
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    if (argc < 2) {
	printf("usage: %s <connection file>\n", argv[0]);
	MPI_Abort(MPI_COMM_WORLD, -1);
    }

    init_state(argv[1]);

    /* make input arrays the windows */
    MPI_Win_create(inarray, inarray_count*sizeof(int),sizeof(int),
		   MPI_INFO_NULL, MPI_COMM_WORLD, &win); 
    setup_groups();		/* only needed for pscw */

    for (itercount = 0; itercount < max_steps; itercount++) {
	/* dump_window(); */
	compute_state();
	printf("state for rank %d at iteration %d is %d\n",
	       myrank, itercount, state);
	MPI_Win_post(ingroup, 0, win);
	MPI_Win_start(outgroup, 0, win);
	if (ready_to_fire()) {
       /* SLIDE: pNeo Code Walkthrough */
	    output_spikes();
	    reset_state();
	}
	MPI_Win_complete(win);
	MPI_Win_wait(win);
    }

    MPI_Win_free(&win);
    MPI_Finalize();
    return 0;
}

void init_state(char *filename)
{
#   define MAX_CONNECTIONS 10000
#   define MAX(x,y) ((x) > (y) ? x : y)

    /* array of connections described in connection file */
    typedef struct {
	int source;
	int dest;
    } connection;

    connection connarray[MAX_CONNECTIONS];

    FILE *confile;
    int i, j, k, n, connarray_count, dispcnt;
    int maxpair, maxcell;

    if ((confile = fopen(filename, "r")) == NULL) {
	printf("could not open connection file %s\n", filename);
/* SLIDE: pNeo Code Walkthrough */
	MPI_Abort(MPI_COMM_WORLD, -1);
    }

    /* The following code should be executed only by process 0,
     * which then should broadcast the connarray */
    maxcell = connarray_count = i = 0;
    inarray_count = outarray_count = 0;

    while ((fscanf(confile, "%d %d", &connarray[i].source,
		   &connarray[i].dest)) == 2) {
	connarray_count++; 
	maxpair = MAX(connarray[i].source, connarray[i].dest);
	if (maxpair > maxcell)
	    maxcell = maxpair;
	if (connarray[i].dest == myrank)
	    inarray_count++;
	if (connarray[i].source == myrank)
	    outarray_count++;
	i++;
    }
    if (maxcell > numprocs-1) {
	printf("%d processes needed for file %s\n",
	       maxcell+1, filename);
	MPI_Abort(MPI_COMM_WORLD, -1);
    }

    /* dump connarray, for debugging */
    if (myrank == 0) {
	printf("global connections:\n");
	for (i = 0; i < connarray_count; i++)
	    printf("%d %d\n", connarray[i].source, 
/* SLIDE: pNeo Code Walkthrough */
		   connarray[i].dest);
    }

    inarray  = (int *)
	           malloc(inarray_count * sizeof(int));
    outarray = (outconnection *)
	           malloc(outarray_count * sizeof(outconnection));
    /* the foll. two arrays are only used in the pscw version */
    inranks  = (int *) malloc(inarray_count * sizeof(int));
    outranks = (int *) malloc(outarray_count * sizeof(int));

    for (i = j = k = 0 ; i < connarray_count; i++) {
	if (connarray[i].dest == myrank) {
	    inranks[j] = connarray[i].source; 
	    j++;
	}
	inarray_count = j;

        if (connarray[i].source == myrank) {
	    dispcnt = 0;
	    for (n = 0; n < connarray_count; n++) {
		if (connarray[n].dest == connarray[i].dest) {
		    if (connarray[n].source == myrank) 
			break;
		    else
			dispcnt++; /* increment counter on
				      destination process i */
		}
	    }
	    outarray[k].dest = connarray[i].dest;
	    outarray[k].disp = dispcnt;
/* SLIDE: pNeo Code Walkthrough */
	    outranks[k] = connarray[i].dest; /* pscw only */
	    k++;
	}
	outarray_count = k;
    }

    /* dump_local_arrays(); */

    state = (myrank + 5) % numprocs; /* essentially random
					for this example */
    for (j = 0; j < inarray_count; j++)
	inarray[j] = 0;		/* no incoming spikes to start */

}

void setup_groups()
{
    MPI_Group worldgroup;

    MPI_Comm_group(MPI_COMM_WORLD, &worldgroup);
    MPI_Group_incl(worldgroup, inarray_count, inranks, &ingroup);
    MPI_Group_incl(worldgroup, outarray_count, outranks,
                   &outgroup);
}

       /* SLIDE: pNeo Code Walkthrough */
void compute_state()
{
    int i;
    int num_incoming = 0;

    for (i = 0; i < inarray_count; i++)
	num_incoming += inarray[i];

    state = state + num_incoming + 1;

    for (i = 0; i < inarray_count; i++)
	inarray[i] = 0;		/* reset spikes */
}

void reset_state()
{
    state = 0;
}

int ready_to_fire()
{
    if (state > 4)
	return 1;
    else
	return 0;
}

       /* SLIDE: pNeo Code Walkthrough */
void output_spikes()
{
    int i;
    /* Note: the static declaration here is important. */
    static int spike = 1;	/* constant values for spikes */

    for (i = 0; i < outarray_count; i++) {
	printf("putting spike from %d to %d in iteration %d\n",
	       myrank, outarray[i].dest, itercount);

	MPI_Put(&spike, 1, MPI_INT, outarray[i].dest,
		outarray[i].disp, 1, MPI_INT, win);
    }
}

       /* SLIDE: pNeo Code Walkthrough */
void dump_local_arrays()
{
    int i;
     
    printf("inarray for process %d: ", myrank);
    for (i = 0; i < inarray_count; i++) 
	printf("%d \n", inarray[i]);
    printf("\n");
    printf("inranks for process %d: ", myrank);
    for (i = 0; i < inarray_count; i++) 
	printf("%d ", inranks[i]);
    printf("\n");
    printf("outarray for process %d:\n", myrank);
    for (i = 0; i < outarray_count; i++) 
	printf("%d %d\n", outarray[i].dest,
	       outarray[i].disp);
    printf("outranks for process %d: ", myrank);
    for (i = 0; i < outarray_count; i++) 
	printf("%d ", outranks[i]);
    printf("\n");
}

void dump_window()
{
    int i;
    printf("inarray for rank %d: ", myrank);
    for (i = 0; i < inarray_count; i++)
	printf("%d ", inarray[i]);
    printf("\n");
}
