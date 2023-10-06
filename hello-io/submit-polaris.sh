#!/bin/bash -l
#PBS -A fallwkshp23
#PBS -l walltime=00:10:00
#PBS -l select=1
#PBS -l place=scatter
#PBS -l filesystems=home:eagle
#PBS -q debug
#PBS -N hello-io
#PBS -V

mkdir -p /eagle/fallwkshp23/${USER}

NNODES=$(wc -l < $PBS_NODEFILE)
NRANKS_PER_NODE=32
NTOTRANKS=$(( NNODES * NRANKS_PER_NODE ))

cd $PBS_O_WORKDIR
mpiexec -n $NTOTRANKS --ppn $NRANKS_PER_NODE \
	./hello-mpiio /eagle/fallwkshp23/${USER}/hello.out
