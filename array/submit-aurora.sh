#!/bin/bash
#PBS -l select=1
#PBS -l daos=default
#PBS -A ATPESC2025
#PBS -l walltime=00:15:00
#PBS -N array-io
#PBS -k doe
#PBS -j oe
#PBS -l filesystems=flare:daos_user_fs
#PBS -V

# I like 'set -e' to immediately bail out on first error, but `daos container
# create` will return an error if the pool already exists.  I would like to be
# able to run this script multiple times, so I need to ignore the error
set -uo pipefail

cd ${PBS_O_WORKDIR}

NNODES=`wc -l < $PBS_NODEFILE`
NRANKS=50        # Number of MPI ranks per node
NDEPTH=1        # Number of hardware threads per rank, spacing between MPI ranks on a node
NTOTRANKS=$(( NNODES * NRANKS ))


cd $PBS_O_WORKDIR
# '.' in path not best practice but will save a few headaches
export PATH=.:${PATH}

# Everyone will use this pool (a collection of daos servers)
DAOS_POOL=ATPESC2025
# inside that pool, everyone will create their own containers for applications
# or demonstrations
DAOS_CONT=${USER}-array

# this placement of processes on an Aurora compute node will use all 8 network
# cards and give the highest bandwith -- not needed for this simple demo but
# helps with non-trivial workloads

binding="list:4:56:5:57:6:58:7:59:8:60:9:61:10:62:11:63:12:64:13:65:14:66:15:67:16:68:17:69:18:70:19:71:20:72:21:73:22:74:23:75:24:76:25:77:26:78:27:79:28:80:29:81:30:82:31:83:32:84:33:85:34:86:35:87:36:88:37:89:38:90:39:91:40:92:41:93:42:94:43:95:44:96:45:97:46:98:47:99:48:100:49:101:50:102:51:103:1:53:2:54:3:55"


if ! $(daos container query $DAOS_POOL $DAOS_CONT >/dev/null 2>&1) ; then
	daos container create --type POSIX $DAOS_POOL $DAOS_CONT
fi

launch-dfuse.sh ${DAOS_POOL}:${DAOS_CONT}

# these all take the same command line arguments -- a file name -- so you can
# simply change the executable here
PROGRAM=array-mpiio-write

mpiexec -np ${NTOTRANKS} -ppn ${NRANKS} --cpu-bind ${binding} --no-vni  \
	./${PROGRAM} /tmp/${DAOS_POOL}/${DAOS_CONT}/array-mpiio.out
