#!/bin/bash -l
#PBS -A ATPESC2024
#PBS -l walltime=00:10:00
#PBS -l select=1
#PBS -l place=scatter
#PBS -l filesystems=home:eagle
#PBS -q debug
#PBS -N hello-io
#PBS -V

OUTPUT=/eagle/radix-io/${USER}/hello
mkdir -p ${OUTPUT}

NNODES=$(wc -l < $PBS_NODEFILE)
NRANKS_PER_NODE=32
NTOTRANKS=$(( NNODES * NRANKS_PER_NODE ))

cd $PBS_O_WORKDIR

mpiexec -n $NTOTRANKS -ppn $NRANKS_PER_NODE \
	./hello-pnetcdf ${OUTPUT}/hello-pnetcdf.nc
