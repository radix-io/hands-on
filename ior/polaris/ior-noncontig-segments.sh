#!/bin/bash -l
#PBS -A CSC250STDM12
#PBS -k doe
#PBS -l walltime=1:00:00
#PBS -l filesystems=home:eagle
#PBS -l select=2
#PBS -l place=scatter

#PBS -N ior
# export all variables to this script
#PBS -V

set -ueo pipefail

#module swap PrgEnv-intel PrgEnv-gnu

cd ${PBS_O_WORKDIR}
NNODES=`wc -l < $PBS_NODEFILE`
NRANKS_PER_NODE=32
NDEPTH=1
NTHREADS=1
NTOTRANKS=$(( NNODES * NRANKS_PER_NODE ))

IOR=${HOME}/soft/polaris/ior-4.0.0
PATH=${IOR}/bin:${PATH}
OUTPUT=/eagle/ATPESC2024/usr/$USER/ior/noncontig-coll
mkdir -p ${OUTPUT}
lfs setstripe -c -1 ${OUTPUT}

# demonstrate the impact of collective I/O optimizations

# optional timing information from Cray MPI implementation internal timers
#export  MPICH_MPIIO_TIMERS=1
#export  MPICH_MPIIO_STATS=1

# for slingshot 10, the very first collective operation might take a long time.
# pay that cost earlier (and ends up being a smaller cost, too).
# might be fixed in ss-11

export MPICH_OFI_STARTUP_CONNECT=1

# demonstrate the impact of data sieving
# look at writes only
# first, independent operations with default hints (data sieving enabled for reads and writes)
# '10000' segments will time out in this configuration

for segs in 1 100 1000 5000; do
    mpiexec -n ${NTOTRANKS} --ppn ${NRANKS_PER_NODE} --depth=${NDEPTH} --cpu-bind depth ior --mpiio.showHints -a MPIIO -w \
	--mpiio.useFileView --mpiio.useStridedDatatype -i 3 -b $((10000000/$segs)) -t $((10000000/$segs)) -s $segs \
	-o ${OUTPUT}/ior-noncontig-$segs.out
done

# compare to time/statistics if we disable data sieving with the
# 'romio_ds_write' hint set to 'disable' instead of default 'automatic'
for segs in 1 100 1000 5000 10000; do
    mpiexec -n ${NTOTRANKS} --ppn ${NRANKS_PER_NODE} --depth=${NDEPTH} --cpu-bind depth -env IOR_HINT__MPI__romio_ds_write=disable \
        ior --mpiio.showHints -a MPIIO -w \
	--mpiio.useFileView --mpiio.useStridedDatatype -i 3 -b $((10000000/$segs)) -t $((10000000/$segs)) -s $segs \
	-o ${OUTPUT}/ior-noncontig-nods-$segs.out
done

# finally, how does collective impact these numbers?  should be pretty flat.
for segs in 1 100 1000 5000 10000; do
    mpiexec -n ${NTOTRANKS} --ppn ${NRANKS_PER_NODE} --depth=${NDEPTH} --cpu-bind depth \
        ior --mpiio.showHints -a MPIIO -w -c \
	--mpiio.useFileView --mpiio.useStridedDatatype -i 3 -b $((10000000/$segs)) -t $((10000000/$segs)) -s $segs \
	-o ${OUTPUT}/ior-noncontig-coll-$segs.out
done

# these Cray hints might have a big impact on collective I/O performance.  From
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
#export  IOR_HINT__MPI__cray_cb_write_lock_mode=2
#export  IOR_HINT__MPI__cb_nodes_multiplier=2
# mode=2 multiplier=4 gave worse performance on two polaris nodes:  maybe it
# helps more if we have more nodes? larger scales?
# mode=2 multipler=2 didn't really change things either.
