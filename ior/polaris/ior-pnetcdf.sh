#!/bin/bash
#PBS -A ATPESC2024
#PBS -k doe
#PBS -l walltime=00:10:00
#PBS -l place=scatter
#PBS -l filesystems=home:eagle

# your environment when you submit this script is what the script will see --
# usually what you want!
#PBS -V
set -ueo pipefail


NNODES=$(wc -l < $PBS_NODEFILE)

IOR=/home/robl/soft/polaris/ior-4.0.0
PATH=${IOR}/bin:${PATH}
OUTPUT=/eagle/ATPESC2024/usr/$USER/ior

# cray workaround: first collective I/O routine can look strangely expensive in
# some cases.  do more wire-up in MPI_Init
export MPICH_OFI_STARTUP_CONNECT=1
# migth be needed if you see this error:
#     MPIDI_CRAY_init: GPU_SUPPORT_ENABLED is requested, but GTL library is not linked
MPICH_GPU_SUPPORT_ENABLED=1

# experiment: collect DXT traces of collective I/O and pnetcdf
export DXT_ENABLE_IO_TRACE=1

# I'd like the darshan report to highlight some collective I/O optimizaitons so
# I am deliberately picking a smaller stripe count.
mkdir -p $OUTPUT
lfs setstripe -c 4 $OUTPUT

mpiexec --np $((32*NNODES)) \
  ior --mpiio.showHints --ncmpi.showHints -c -a NCMPI \
       -b 1000000 -t 1000000 -o ${OUTPUT}/ior-pnetcdf.out
