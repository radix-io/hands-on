#!/bin/bash
#SBATCH --qos=debug
#SBATCH --time=00:15:00
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=128
#SBATCH --constraint=cpu

module load cray-hdf5-parallel

# make sure you point to your h5bench installation
H5BENCH_DIR=

# make sure h5bench binaries are accessible
export PATH=${H5BENCH_DIR}/bin:$PATH

# make sure scratch dir exists
SCRATCHDIR=$SCRATCH/h5bench-storage-$SLURM_JOB_ID

mkdir -p $SCRATCHDIR

cp h5bench.json ${SLURM_JOB_ID}-h5bench.json

sed -i "s|SCRATCHDIR|${SCRATCHDIR}|g" ${SLURM_JOB_ID}-h5bench.json

# submit job
h5bench --debug --abort-on-failure --validate-mode ${SLURM_JOB_ID}-h5bench.json

# delete the new file to avoid exhausing the ATPESC quota
rm -rf $SCRATCHDIR/*.h5