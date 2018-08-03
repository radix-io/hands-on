#include <stdlib.h>
#include "array.h"

int main()
{
    /* MPI-friendly allocation: faster and easier to describe */
    int *array;
    array = malloc(XDIM*YDIM*sizeof(*array));

    /* not MPI-friendly: describing this memory region will require a more
     * complictaed data type description */
    int **annoying;
    int i;
    annoying = malloc(YDIM*sizeof(*array));
    for (i=0; i<YDIM; i++)
        annoying[i] = malloc(XDIM*sizeof(*array));

    free(array);
    free(annoying);

    return 0;
}
