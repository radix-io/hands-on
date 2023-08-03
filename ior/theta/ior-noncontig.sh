#!/bin/bash
#COBALT -t 10
#COBALT -n 4
#COBALT --attrs mcdram=cache:numa=quad
#COBALT -A ATPESC2022

set -ueo pipefail

module swap PrgEnv-intel PrgEnv-gnu


IOR=${HOME}/soft/cray/ior-main
PATH=${IOR}/bin:${PATH}
OUTPUT=/grand/ATPESC2022/usr/$USER/ior

# demonstrate the impact of data sieving
# look at writes only
# first, independent operations with default hints (data sieving enabled for reads and writes)
# '10000' segments will time out in this configuration

for segs in 1 100 1000 5000; do
    aprun -N 48 -n $((48*4)) \
      ior --mpiio.showHints  -a MPIIO -w \
           --mpiio.useFileView --mpiio.useStridedDatatype -b $((10000000/$segs)) -t $((10000000/$segs)) -s $segs \
           -o ${OUTPUT}/ior-noncontig-$segs.out
done

# compare to time/statistics if we disable data sieving with the
# 'romio_ds_write' hint set to 'disable' instead of default 'automatic'
for segs in 1 100 1000 10000; do
    aprun -N 48 -n $((48*4)) -e IOR_HINT__MPI__romio_ds_write=disable \
      ior --mpiio.showHints  -a MPIIO -w \
           -e IOR_HINT__MPI__romio_ds_write=disable \
           --mpiio.useFileView --mpiio.useStridedDatatype -b 1000 -t 1000 -s 10000 \
           -o ${OUTPUT}/ior-noncontig-nods-$segs.out
done
