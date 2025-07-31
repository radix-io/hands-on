#!/bin/bash
#PBS -l select=1
#PBS -l daos=perf
#PBS -A ATPESC2025
#PBS -l walltime=00:15:00
#PBS -N array-io
#PBS -k doe
#PBS -j oe
#PBS -l filesystems=flare:daos_perf_fs
#PBS -V

# your environment when you submit this script is what the script will see --
# usually what you want!
PBS -V
set -ueo pipefail


NNODES=$(wc -l < $PBS_NODEFILE)
NRANKS=48        # Number of MPI ranks per node
NDEPTH=1        # Number of hardware threads per rank, spacing between MPI ranks on a node
NTOTRANKS=$(( NNODES * NRANKS ))

IOR=${HOME}/soft/ior
PATH=${IOR}/bin:${PATH}

module load daos_perf
# Everyone will use this pool (a collection of daos servers)
DAOS_POOL=ATPESC2025_0
# inside that pool, everyone will create their own containers for applications
# or demonstrations
DAOS_CONT=${USER}-ior-simple

binding="list:4:56:5:57:6:58:7:59:8:60:9:61:10:62:11:63:12:64:13:65:14:66:15:67:16:68:17:69:18:70:19:71:20:72:21:73:22:74:23:75:24:76:25:77:26:78:27:79:28:80:29:81:30:82:31:83:32:84:33:85:34:86:35:87:36:88:37:89:38:90:39:91:40:92:41:93:42:94:43:95:44:96:45:97:46:98:47:99:48:100:49:101:50:102:51:103:1:53:2:54:3:55"

if ! $(daos container query $DAOS_POOL $DAOS_CONT >/dev/null 2>&1) ; then
	daos container create --type POSIX $DAOS_POOL $DAOS_CONT
fi

launch-dfuse_perf.sh ${DAOS_POOL}:${DAOS_CONT}



# experiment: compare collective I/O behavior between Polaris (Lustre) and Ascent (GPFS)
# requires forcing collective I/O on ascent, but cray's MPI-IO will do the right thing

export LD_PRELOAD=${DARSHAN_RUNTIME_ROOT}/lib/libdarshan.so
mpiexec --np 48 -ppn ${NRANKS} --cpu-bind ${binding} --no-vni \
  --env IOR_HINT__MPI__romio_cb_write=enable \
  ior --mpiio.showHints -c -a MPIIO \
       -b 1000000 -t 1000000 -o /tmp/${DAOS_POOL}/${DAOS_CONT}/ior-simple.out
