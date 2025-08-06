/*
 * Code example for using DAOS APIs directly.
 * Recommended if you want to optimize your I/O specifically for DAOS.
 *
 * Build:
 *   $(CC) -o daos-hello-world daos-hello-world.c -ldaos -ldaos_common -luuid
 */
#include <stdlib.h>
#include <daos.h>

int main(int argc, char **argv)
{
    int rc = 0;

    /*
     * Initialize the daos library
     */
    rc = daos_init();
    if (rc)
    {
        fprintf(stderr, "daos_init() failed with %d\n", rc);
        goto error;
    }

    printf("hello, world\n");

error:
    /*
     * Finalize the daos library
     */
    rc = daos_fini();
    if (rc)
    {
        fprintf(stderr, "daos_fini() failed with %d\n", rc);
    }

    return(rc);
}
