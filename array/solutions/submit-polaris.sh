#!/bin/bash -l
#PBS -A ATPESC2023
#PBS -l walltime=00:10:00
#PBS -l select=1
#PBS -l place=scatter
#PBS -l filesystems=home:grand
#PBS -q debug
#PBS -N io-day
#PBS -V

set -euo pipefail

NNODES=$(wc -l < $PBS_NODEFILE)
NRANKS_PER_NODE=32
NTOTRANKS=$(( NNODES * NRANKS_PER_NODE ))

cd $PBS_O_WORKDIR
# '.' in path not best practice but will save a few headaches
export PATH=.:${PATH}


TRAINING_DIR=/grand/ATPESC2023/usr/$USER

# shell expansion syntax: if there is no second argument, use the file name
# 'testfile' as default value
FILENAME=${2:-testfile}

mkdir -p ${TRAINING_DIR}

echo "writing to ${TRAINING_DIR}/${FILENAME}"
mpiexec -n ${NTOTRANKS} --ppn ${NRANKS_PER_NODE} \
  $APPLICATION ${TRAINING_DIR}/${FILENAME}
