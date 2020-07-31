#!/bin/sh
#BSUB -P gen139
#BSUB -W 0:5
#BSUB -nnodes 1
#BSUB -J ior-demo

jsrun -r 1 -c ALL_CPUS -a 48 \
        --env IOR_HINT__MPI__romio_cb_write=enable\
        --env IOR_HINT__MPI__romio_cb_read=enable\
        ./src/ior  \
       -H -c -a MPIIO \
       -b 1000000 -t 1000000 -o /ccsopen/proj/gen139/data-and-io/$USER/ior-simple.out
