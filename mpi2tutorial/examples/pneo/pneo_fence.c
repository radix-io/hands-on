/* SLIDE: pNeo Code Walkthrough */
/*  This is an abstraction of the pNeo brain simulation program,
    written to illustrate the MPI one-sided operations.

    Run with

      pneo <filename>

    where <filename> is a file of connections, one on each line
    consisting of a blank-separated pair of integers in character
    format.  Only one cell per proc is modelled in this version.

    This is the fence version.

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





/* SLIDE: pNeo Code Walkthrough */
/* Connections to other cells are prepresented by an array of
 * inputs (inconnections) and an array of outputs (outconnections)
 * A connection array entry contains the rank of the other process
 * and the values of the incoming or outgoing spikes.  The input
 * connection arrays are the windows. The outconnections also
 * contain the displacement into the destination window of the
 * connection.
 */

typedef struct {
    int source;
    int inspike;
} inconnection;

inconnection *inarray;		/* array of inputs */
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

int numprocs, myrank;
int itercount;
/* SLIDE: pNeo Code Walkthrough */
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

    /*initialize connection arrays and cell state */
    init_state(argv[1]);

    /* make input arrays the windows */
    MPI_Win_create(inarray, inarray_count * sizeof(int),
                   sizeof(int), MPI_INFO_NULL,
                   MPI_COMM_WORLD, &win); 

    for (itercount = 0; itercount < max_steps; itercount++) {
	compute_state();
	MPI_Win_fence(0, win);
	if (ready_to_fire()) {
	    output_spikes();
	    reset_state();
	}
	MPI_Win_fence(0, win);
    }

/* SLIDE: pNeo Code Walkthrough */
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
	MPI_Abort(MPI_COMM_WORLD, -1);
    }

    /* The following code should be executed only by process 0,
     * which then should broadcast the connarray */
    maxcell = connarray_count = i = 0;
    inarray_count = outarray_count = 0;
/* SLIDE: pNeo Code Walkthrough */
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
    for (i = 0; i < connarray_count; i++) {
	printf("%d %d\n", connarray[i].source, connarray[i].dest);
    }

    inarray  = (inconnection *)
	           malloc(inarray_count * sizeof(inconnection));
    outarray = (outconnection *)
	           malloc(outarray_count * sizeof(outconnection));

    for (i = j = k = 0 ; i < connarray_count; i++) {
	if (connarray[i].dest == myrank) {
	    inarray[j].source = connarray[i].source;
/* SLIDE: pNeo Code Walkthrough */
	    inarray[j].inspike = 0;
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
	    k++;
	}
	outarray_count = k;
    }

    dump_local_arrays();

    state = (myrank + 5) % numprocs; /* essentially random
					for this example */
}

void compute_state()
{
/* SLIDE: pNeo Code Walkthrough */
    int i;
    int num_incoming = 0;
    
    for (i = 0; i < inarray_count; i++) {
	num_incoming += inarray[i].inspike;
    }

    state = state + num_incoming + 1;
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

void output_spikes()
{
    int i;
    static int spike = 1;

    for (i = 0; i < outarray_count; i++) {
	printf("putting spike from %d to %d in interation %d\n",
	       myrank, outarray[i].dest, itercount);
/* SLIDE: pNeo Code Walkthrough */
	MPI_Put(&spike, 1, MPI_INT, outarray[i].dest,
		outarray[i].disp, 1, MPI_INT, win);
    }
}

void dump_local_arrays()
{
    int i;
     
    printf("inarray for process %d:\n", myrank);
    for (i = 0; i < inarray_count; i++) 
	printf("%d %d\n", inarray[i].source, inarray[i].inspike);
    printf("outarray for process %d:\n", myrank);
    for (i = 0; i < outarray_count; i++) 
	printf("%d %d\n", outarray[i].dest, outarray[i].disp);
}

