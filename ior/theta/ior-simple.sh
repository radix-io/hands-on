#!/bin/bash
#COBALT -t 10
#COBALT -n 1
#COBALT --attrs mcdram=cache:numa=quad
#COBALT -A ATPESC2022

module swap PrgEnv-intel PrgEnv-gnu


IOR=${HOME}/soft/cray/ior-4.0.0rc1
PATH=${IOR}/bin:${PATH}
OUTPUT=/grand/ATPESC2022/usr/$USER/ior

# experiment: compare collective I/O behavior between Theta (Lustre) and Ascent (GPFS)
# requires forcing collective I/O on ascent, but cray's MPI-IO will do the right thing
#	--env IOR_HINT__MPI__romio_cb_write=enable

aprun -n 48 \
  ior --mpiio.showHints -c -a MPIIO \
       -b 1000000 -t 1000000 -o /lus/theta-fs0/projects/radix-io/$USER/ior-simple.out
