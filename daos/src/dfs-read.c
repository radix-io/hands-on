#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <mpi.h>

#include "util.h"
#include "array.h"

#include <daos.h>
#include <daos_fs.h>

int read_data(char *filename, dfs_t *dfs)
{
    /* We will store whatever additional information we want to retain about
     * the data in this struct */
    science data = {
        .row = YDIM,
        .col = XDIM,
        .iter = 1
    };

    int *array;
    int ret=0;
    int rank;
    int ranks;
    int iterations=ITER;
    size_t totalsz;
    off_t off = 0;
    double stime,etime,ttime,maxtime;
    dfs_obj_t *obj;
    d_iov_t global;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);

    /* defined in util.c, this routine will give us an initialized region of memory */
    array = buffer_create(0, XDIM, YDIM);


    if (rank == 0)
    {
    //obj_class = OC_EC2P1GX;
    daos_oclass_id_t obj_class = OC_SX;
    int chunk_size = 512*1024;
    ret = dfs_open(dfs, NULL, filename, S_IFREG|S_IRUSR|S_IWUSR,
                   O_RDONLY,
                  obj_class, chunk_size, NULL, &obj);
    if (ret) { fprintf(stderr, "dfs open failed\n"); goto fn_error; }

    global.iov_len = 0;
    global.iov_buf_len = 0;
    global.iov_buf = 0;

    dfs_obj_local2global(dfs, obj, &global);
    global.iov_buf = malloc(global.iov_buf_len);
    global.iov_len = global.iov_buf_len;
    dfs_obj_local2global(dfs, obj, &global);

    ret = MPI_Bcast(&global.iov_buf_len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) goto fn_error;

    ret = MPI_Bcast(global.iov_buf, global.iov_buf_len, MPI_BYTE, 0, MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) goto fn_error;
 
    }
    else
    {

    ret = MPI_Bcast(&global.iov_buf_len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) goto fn_error;

    global.iov_len = global.iov_buf_len;
    global.iov_buf = malloc(global.iov_buf_len);

    ret = MPI_Bcast(global.iov_buf, global.iov_buf_len, MPI_BYTE, 0, MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) goto fn_error;

    ret = dfs_obj_global2local(dfs, 0, global, &obj);
    if (ret) goto fn_error;
    
    }

    totalsz = (sizeof(data) + sizeof(int)*XDIM*YDIM) * iterations;

    /* get the offset for each rank */
    ret = MPI_Exscan(&totalsz, &off, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) goto fn_error;

    MPI_Barrier(MPI_COMM_WORLD);
    stime = MPI_Wtime();

    for (int i = 0; i < iterations; i++)
    {

    data.iter = i+1;

    /* we have two  memory regions -- the data and the struct containing
     * information about the data -- so we have to issue two write calls */
    d_sg_list_t sgl;
    d_iov_t iov[2];
    sgl.sg_nr = 2;
    sgl.sg_nr_out = 0;
    sgl.sg_iovs = &iov[0];
    daos_size_t read;

    d_iov_set(&iov[0], &data, sizeof(data));
    d_iov_set(&iov[1], array, XDIM*YDIM*sizeof(int));

    ret = dfs_read(dfs, obj, &sgl, off, &read, NULL);
    if (ret) { fprintf(stderr, "dfs write failed\n"); goto fn_error; }

    if ((data.iter != i+1) ||
        (data.row != XDIM) ||
        (data.col != YDIM)) { printf("read error: %i %i, %i %i, %i %i\n",
                                 i, data.iter,
                                 XDIM, data.row,
                                 YDIM, data.col); goto fn_error; }

    off += sizeof(data) + XDIM*YDIM*sizeof(int);

    }

    ret = dfs_release(obj);
    if (ret) goto fn_error;

    MPI_Barrier(MPI_COMM_WORLD);
    etime = MPI_Wtime();
    ttime = etime - stime;
    MPI_Reduce(&ttime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
       printf("total time; %lf second\n", maxtime);
       printf("data size: %" PRId64 " KiB\n", sizeof(int)*XDIM*YDIM/1024);
       printf("total size: %" PRId64 " KiB\n", totalsz * ranks / 1024);
       printf("BW: %lf MiB/s\n", (double)(totalsz * ranks) / 1024 / 1024 / maxtime);
       printf("IOps %lf Kops\n", (double)(iterations*2*ranks) / 1000 / maxtime);
    }

    buffer_destroy(array);
    return ret;

fn_error:
    perror("Error:");
    MPI_Abort(MPI_COMM_WORLD, ret);
    return ret;
}

int main(int argc, char **argv)
{
    int ret;
    int rank;
    dfs_t *dfs;
    d_iov_t global = {0};

    ret = MPI_Init(&argc, &argv);

    ret = MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ret = dfs_init();
    if (ret) MPI_Abort(MPI_COMM_WORLD, ret);

    if (rank == 0)
    {

        ret = dfs_connect(getenv("DAOS_POOL"), NULL, getenv("DAOS_CONT"), O_RDWR, NULL, &dfs);
        if (ret) { fprintf(stderr, "dfs connect failed\n"); MPI_Abort(MPI_COMM_WORLD, ret); }
    
	memset(&global, 0, sizeof(global));
        dfs_local2global_all(dfs, &global);

        global.iov_len = global.iov_buf_len;
        global.iov_buf = malloc(global.iov_buf_len);

        dfs_local2global_all(dfs, &global);

        ret = MPI_Bcast(&global.iov_buf_len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
        if (ret != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, ret);
        
        ret = MPI_Bcast(global.iov_buf, global.iov_buf_len, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (ret != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, ret);

    }
    else
    {

        global.iov_len = 1;

        ret = MPI_Bcast(&global.iov_buf_len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
        if (ret != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, ret);
        
        global.iov_len = global.iov_buf_len;
        global.iov_buf = malloc(global.iov_buf_len);

        ret = MPI_Bcast(global.iov_buf, global.iov_buf_len, MPI_BYTE, 0, MPI_COMM_WORLD);

        dfs_global2local_all(0, global, &dfs);

    }

    ret = read_data("testfile", dfs);

    ret = dfs_disconnect(dfs);

    free(global.iov_buf);
  
    ret = dfs_fini();

    ret = MPI_Finalize(); 
}
