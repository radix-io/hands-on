#!/bin/bash
#PBS -A ATPESC2023
#PBS -k doe
#PBS -l walltime=00:10:00
#PBS -l place=scatter
#PBS -l filesystems=home:eagle

# your environment when you submit this script is what the script will see --
# usually what you want!
#PBS -V
set -ueo pipefail


NNODES=$(wc -l < $PBS_NODEFILE)

IOR=${HOME}/soft/polaris/ior-4.0.0rc1
PATH=${IOR}/bin:${PATH}
OUTPUT=/eagle/ATPESC2023/usr/$USER/ior

mkdir -p $OUTPUT
lfs setstripe -c -1 $OUTPUT

# experiment: compare collective I/O behavior between Polaris (Lustre) and Ascent (GPFS)
# requires forcing collective I/O on ascent, but cray's MPI-IO will do the right thing
#	--env IOR_HINT__MPI__romio_cb_write=enable

mpiexec --np 48 \
  ior --mpiio.showHints -c -a MPIIO \
       -b 1000000 -t 1000000 -o ${OUTPUT}/ior-simple.out
