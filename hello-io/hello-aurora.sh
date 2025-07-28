#!/bin/bash
#PBS -l select=1
#PBS -l daos=perf
#PBS -A ATPESC2025
#PBS -l walltime=00:15:00
#PBS -N hello-io
#PBS -k doe
#PBS -j oe
#PBS -l filesystems=flare:daos_perf_fs
#PBS -V

cd ${PBS_O_WORKDIR}

NNODES=`wc -l < $PBS_NODEFILE`
NRANKS=16        # Number of MPI ranks per node
NDEPTH=1        # Number of hardware threads per rank, spacing between MPI ranks on a node
NTOTRANKS=$(( NNODES * NRANKS ))

module load daos_perf

# Everyone will use this pool (a collection of daos servers)
DAOS_POOL=ATPESC2025_0
# inside that pool, everyone will create their own containers for applications
# or demonstrations
DAOS_CONT=${USER}-hello


# this placement of processes on an Aurora compute node will use all 8 network
# cards and give the highest bandwith -- not needed for this simple demo but
# helps with non-trivial workloads

binding="list:4:56:5:57:6:58:7:59:8:60:9:61:10:62:11:63:12:64:13:65:14:66:15:67:16:68:17:69:18:70:19:71:20:72:21:73:22:74:23:75:24:76:25:77:26:78:27:79:28:80:29:81:30:82:31:83:32:84:33:85:34:86:35:87:36:88:37:89:38:90:39:91:40:92:41:93:42:94:43:95:44:96:45:97:46:98:47:99:48:100:49:101:50:102:51:103:1:53:2:54:3:55"


daos container create --type POSIX $DAOS_POOL $DAOS_CONT
launch-dfuse_perf.sh ${DAOS_POOL}:${DAOS_CONT}

echo "==== contiguous in memory and file"
mpiexec -np ${NTOTRANKS} -ppn ${NRANKS} --cpu-bind ${binding} --no-vni  \
	./hello-mpiio /tmp/${DAOS_POOL}/${DAOS_CONT}/hello.out

echo "cat /tmp/${DAOS_POOL}/${DAOS_CONT}/hello.out"
cat /tmp/${DAOS_POOL}/${DAOS_CONT}/hello.out

echo "==== noncontiguous in memory"
mpiexec -np ${NTOTRANKS} -ppn ${NRANKS} --cpu-bind ${binding} --no-vni  \
	./hello-mpiio-noncontig /tmp/${DAOS_POOL}/${DAOS_CONT}/hello-noncontig.out
echo "cat /tmp/${DAOS_POOL}/${DAOS_CONT}/hello-noncontig.out"
cat /tmp/${DAOS_POOL}/${DAOS_CONT}/hello-noncontig.out

echo "==== noncontiguous in file"
mpiexec -np ${NTOTRANKS} -ppn ${NRANKS} --cpu-bind ${binding} --no-vni  \
	./hello-mpiio-view /tmp/${DAOS_POOL}/${DAOS_CONT}/hello-view.out
echo "cat /tmp/${DAOS_POOL}/${DAOS_CONT}/hello-view.out"
cat /tmp/${DAOS_POOL}/${DAOS_CONT}/hello-view.out
