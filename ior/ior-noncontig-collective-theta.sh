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

# demonstrate the impact of collective I/O optimizations


for segs in 1 100 1000 5000 10000; do
   aprun -N 48 -n $((48*4)) \
      ior --mpiio.showHints  -a MPIIO -c -w \
           --mpiio.useFileView --mpiio.useStridedDatatype -b $((10000000/$segs)) -t $((10000000/$segs)) -s $segs \
           -o ${OUTPUT}/ior-noncontig-coll-$segs.out
done

# these Cray hints have a big impact on collective I/O performance.  From
# intro_mpi man page:
#     cray_cb_write_lock_mode: Sets of non-overlapping extent locks are
#                              acquired ahead of time by all MPI ranks that are
#                              writing the file and the acquired locks are not
#                              expanded beyond the size requested by the ranks
#     cb_nodes_multiplier:     Specifies the number of collective buffering
#                              aggregators (cb_nodes) per OST for Lustre files.
#                              In other words, the number of aggregators is the
#                              stripe count (striping_factor) times the
#                              multiplier.   When the locking mode is 1 or 2, a
#                              multiplier of 2 or more is usually best for
#                              writing the file

export  IOR_HINT__MPI__cray_cb_write_lock_mode=2
export  IOR_HINT__MPI__cb_nodes_multiplier=4

for segs in 1 100 1000 5000 10000; do
   aprun -N 48 -n $((48*4)) \
      ior --mpiio.showHints  -a MPIIO -c -w \
           --mpiio.useFileView --mpiio.useStridedDatatype -b $((10000000/$segs)) -t $((10000000/$segs)) -s $segs \
           -o ${OUTPUT}/ior-noncontig-coll-$segs.out
done
