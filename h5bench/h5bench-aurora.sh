#!/bin/bash
#PBS -l select=1
#PBS -l daos=default
#PBS -A ATPESC2025
#PBS -l walltime=00:15:00
#PBS -N h5bench
#PBS -k doe
#PBS -j oe
#PBS -l filesystems=flare:daos_user_fs
#PBS -V

# I like 'set -e' to immediately bail out on first error, but `daos container
# create` will return an error if the pool already exists.  I would like to be
# able to run this script multiple times, so I need to ignore the error
set -euo pipefail

cd ${PBS_O_WORKDIR}


# make sure you point to your h5bench installation
H5BENCH_DIR=${HOME}/soft/h5bench

# make sure h5bench binaries are accessible
export PATH=${H5BENCH_DIR}/bin:$PATH

NNODES=`wc -l < $PBS_NODEFILE`
NRANKS=50        # Number of MPI ranks per node
NDEPTH=1        # Number of hardware threads per rank, spacing between MPI ranks on a node
NTOTRANKS=$(( NNODES * NRANKS ))


DAOS_POOL=ATPESC2025
DAOS_CONT=${USER}-h5bench

daos container create --type POSIX $DAOS_POOL $DAOS_CONT || true
launch-dfuse.sh ${DAOS_POOL}:${DAOS_CONT}

# make sure scratch dir exists
SCRATCHDIR=/tmp/${DAOS_POOL}/${DAOS_CONT}/h5bench-storage

mkdir -p $SCRATCHDIR

cp atpsec.json ${PBS_JOBID_SHORT}-h5bench.json

sed -i "s|SCRATCHDIR|${SCRATCHDIR}|g" ${PBS_JOBID_SHORT}-h5bench.json

# submit job
h5bench --debug --abort-on-failure --validate-mode ${PBS_JOBID_SHORT}-h5bench.json
