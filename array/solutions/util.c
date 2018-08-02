#include <stdlib.h>

int * buffer_create(int seed, int x, int y)
{
    int i;
    int *buffer = malloc(x*y*sizeof(int));
    for (i=0; i<x*y; i++)
    {
        buffer[i] = seed*10+i;
    }
    return buffer;

}

void buffer_destroy(int *buffer)
{
    free(buffer);
}

