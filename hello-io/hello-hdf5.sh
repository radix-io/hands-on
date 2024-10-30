#!/bin/bash -l
#PBS -A alcf_training
#PBS -l walltime=00:10:00
#PBS -l select=1
#PBS -l place=scatter
#PBS -l filesystems=home:eagle
#PBS -q HandsOnHPC
#PBS -N hello-hdf5
#PBS -V

OUTPUT=/grand/alcf_training/HandsOnHPC24/$USER
mkdir -p ${OUTPUT}

NNODES=$(wc -l < $PBS_NODEFILE)
NRANKS_PER_NODE=32
NTOTRANKS=$(( NNODES * NRANKS_PER_NODE ))

cd $PBS_O_WORKDIR

mpiexec -n $NTOTRANKS -ppn $NRANKS_PER_NODE \
	./hello-hdf5 ${OUTPUT}/hello-hdf5.h5
