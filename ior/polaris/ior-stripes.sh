#!/bin/bash -l
#PBS -A ATPESC2023
#PBS -k doe
#PBS -l walltime=1:00:00
#PBS -l filesystems=home:grand
#PBS -l select=8
#PBS -l place=scatter

#PBS -N ior
# export all variables to this script
#PBS -V

set -ueo pipefail

#module swap PrgEnv-intel PrgEnv-gnu

cd ${PBS_O_WORKDIR}
NNODES=`wc -l < $PBS_NODEFILE`
# Testing I/O performance, not trying to mimmic an application.  Only need a
# few clients to saturate the link
NRANKS_PER_NODE=16
NDEPTH=1
NTHREADS=1
NTOTRANKS=$(( NNODES * NRANKS_PER_NODE ))

IOR=${HOME}/soft/polaris/ior-4.0.0rc1
PATH=${IOR}/bin:${PATH}
OUTPUT=/grand/ATPESC2023/usr/$USER/ior

# demonstrate the impact of striping factor

# some additional timing information can sometimes be useful in more complex
# applications.
#export  MPICH_MPIIO_TIMERS=1
#export  MPICH_MPIIO_STATS=1

# benchmarking to answer the striping question
# - independent i/o:  32 processes will be more than one node network link can
#   handle, but we're just looking at file system performance
# - sync writes (-Y) to avoid caching writes
# - reorder reads (-C 32) to avoid reading from page cache
# - need to delete and recreate the file each time: striping can only be set at creation

# on Polaris the 'grand' file system has 160 lustre servers.  Other Lustre
# deployments might have more or less.  The last "-1" value will request all
# available servers.
# additionally! Lustre has an "overstripe" feature: we can put multiple stripes
# on an OST, so while '160' is "every server", we can crank it up even higher
# but! overstriping does not seem to work with MPI-IO hints so we instead create a directory for each striping configuration
# august 2023: there's a new 'overstriping_factor' but not available until 8.1.27
# - stripe of '1' is so bad that it eats up all the allocation
#for stripe in 1 5 20 80 160 320 640; do
for stripe in 5 20 80 160 320 640; do
    rm  -rf ${OUTPUT}/$stripe/
    mkdir -p ${OUTPUT}/$stripe
    lfs setstripe -C $stripe ${OUTPUT}/$stripe
        # -a MPIIO: using MPI-IO so we can pass the "striping_factor" hint
	# -e      : fsync after each write phase: push out dirty data to storage
	# -C      : reorder ranks: read from a different rank than the one that wrote
	# -s      : segments: each client will write to eight regions
	# -i      : repeat experiment five times: lots of variability in I/O
	# -t      : transfer size: how big each request will be
	# -b      : block size:  how big each region will be in the file (needs to be a multiple of transfer size).
    mpiexec -n ${NTOTRANKS} --ppn ${NRANKS_PER_NODE} --depth=${NDEPTH} --cpu-bind depth \
        ior --mpiio.showHints -a MPIIO \
	-e -C -s 8 -i 5 \
	-t 1MiB -b 64MiB -o ${OUTPUT}/${stripe}/ior-stripe-$stripe.out
done

# for comparison, one file-per-process (-F), not-striped run.  should represent
# fastest reads and writes (even if not ideal for subsequent use)
export IOR_HINT__MPI__striping_factor=1
# -a MPIIO: using MPI-IO so we can pass the "striping_factor" hint
# -e      : fsync after each write phase: push out dirty data to storage
# -C      : reorder ranks: read from a different rank than the one that wrote
# -s      : segments: each client will write to eight regions
# -i      : repeat experiment five times: lots of variability in I/O
# -t      : transfer size: how big each request will be
# -b      : block size:  how big each region will be in the file (needs to be a multiple of transfer size)

rm -rf ${OUTPUT}/fpp
mkdir -p ${OUTPUT}/fpp

mpiexec -n ${NTOTRANKS} --ppn ${NRANKS_PER_NODE} --depth=${NDEPTH} --cpu-bind depth \
        ior --mpiio.showHints -a MPIIO \
	-F \
	-e -C -s 8 -i 5 \
	-t 1MiB -b 64MiB -o ${OUTPUT}/fpp/ior-fpp-1.out
