#!/bin/bash
#COBALT -t 10
#COBALT -n 2
#COBALT --attrs mcdram=cache:numa=quad
#COBALT -A ATPESC2022
#COBALT -q ATPESC2022

set -euo pipefail

export n_nodes=$COBALT_JOBSIZE
export n_mpi_ranks_per_node=32
export n_mpi_ranks=$(($n_nodes * $n_mpi_ranks_per_node))
export n_openmp_threads_per_rank=4
export n_hyperthreads_per_core=2
export n_hyperthreads_skipped_between_ranks=4

# '.' in path not best practice but will save a few headaches
export PATH=.:${PATH}

APPLICATION=$1

TRAINING_DIR=/grand/ATPESC2022/$USER

# shell expansion syntax: if there is no second argument, use the file name
# 'testfile' as default value
FILENAME=${2:-testfile}

mkdir -p ${TRAINING_DIR}

echo "writing to ${TRAINING_DIR}/${FILENAME}"
aprun -n $n_mpi_ranks -N $n_mpi_ranks_per_node \
  --env OMP_NUM_THREADS=$n_openmp_threads_per_rank -cc depth \
  -d $n_hyperthreads_skipped_between_ranks \
  -j $n_hyperthreads_per_core \
  $1 ${TRAINING_DIR}/${FILENAME}
