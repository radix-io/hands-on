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


# experiment: collect DXT traces of collective I/O and pnetcdf
export DXT_ENABLE_IO_TRACE=1

# I'd like the darshan report to highlight some collective I/O optimizaitons so
# I am deliberately picking a smaller stripe count.
mkdir -p $OUTPUT
lfs setstripe -c 4 $OUTPUT

mpiexec --np $((32*NNODES)) \
  ior --mpiio.showHints --ncmpi.showHints -c -a NCMPI \
       -b 1000000 -t 1000000 -o ${OUTPUT}/ior-pnetcdf.out
