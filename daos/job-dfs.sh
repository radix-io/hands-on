#!/bin/bash
#PBS -A Aurora_deployment
#PBS -lselect=1
#PBS -lwalltime=30:00
#PBS -k doe
#PBS -lfilesystems=home:daos_user_fs

# Test case for DFS code example

# ranks per node
rpn=4

# threads per rank
threads=1

# nodes per job
nnodes=$(cat $PBS_NODEFILE | wc -l)

# Verify the pool and container are set
if [ -z "$DAOS_POOL" ];
then
    echo "You must set DAOS_POOL"
    exit 1
fi

if [ -z "$DAOS_CONT" ];
then
    echo "You must set DAOS_CONT"
    exit 1
fi

# load daos/base module (if not loaded)
module use /soft/modulefiles
module load daos/base

# print your module list (useful for debugging)
module list

# print your environment (useful for debugging)
#env

# turn on output of what is executed
set -x

# launch dfuse on all compute nodes
# will be launched using pdsh
# arguments:
#   pool:container
# may list multiple pool:container arguments
# will be mounted at:
#   /tmp/<pool>/<container>
launch-dfuse.sh ${DAOS_POOL}:${DAOS_CONT}

# change to submission directory
cd $PBS_O_WORKDIR

# run your job(s)

# these test cases assume 'testfile' is in the CWD
cd /tmp/${DAOS_POOL}/${DAOS_CONT}

echo "write"
mpiexec -np $((rpn*nnodes)) \
	-ppn $rpn \
	-d $threads \
	--cpu-bind numa \
	--no-vni \
	-genvall \
	$HOME/${USER}_cont/src/dfs-write

echo "read"
mpiexec -np $((rpn*nnodes)) \
	-ppn $rpn \
	-d $threads \
	--cpu-bind numa \
	--no-vni \
	-genvall \
	$HOME/${USER}_cont/src/dfs-read

exit 0
