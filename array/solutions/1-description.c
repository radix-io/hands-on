#include <stdlib.h>
#include "array.h"

int main()
{
    int *array;
    array = malloc(XDIM*YDIM*sizeof(*array));
    return 0;
}
