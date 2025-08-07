#!/bin/bash
#PBS -l select=2
#PBS -A ATPESC2025
#PBS -l walltime=00:15:00
#PBS -N ior-pnetcdf
#PBS -k doe
#PBS -j oe
#PBS -l daos=default
#PBS -l filesystems=flare:daos_user_fs
#PBS -V


# your environment when you submit this script is what the script will see --
# usually what you want!
#PBS -V

NNODES=`wc -l < $PBS_NODEFILE`
NRANKS=96          # Number of MPI ranks per node
NDEPTH=1        # Number of hardware threads per rank, spacing between MPI ranks on a node
NTOTRANKS=$(( NNODES * NRANKS ))

IOR=/home/robl/soft/ior
PATH=${IOR}/bin:${PATH}

# first four cores on each cpu are os-dedicated
binding="list:4:56:5:57:6:58:7:59:8:60:9:61:10:62:11:63:12:64:13:65:14:66:15:67:16:68:17:69:18:70:19:71:20:72:21:73:22:74:23:75:24:76:25:77:26:78:27:79:28:80:29:81:30:82:31:83:32:84:33:85:34:86:35:87:36:88:37:89:38:90:39:91:40:92:41:93:42:94:43:95:44:96:45:97:46:98:47:99:48:100:49:101:50:102:51:103"

# experiment: collect DXT traces of collective I/O and pnetcdf
export DXT_ENABLE_IO_TRACE=1

DAOS_POOL=ATPESC2025
DAOS_CONT=ior-pnetcdf

daos container destroy $DAOS_POOL $DAOS_CONT
daos container create --type POSIX $DAOS_POOL $DAOS_CONT
launch-dfuse.sh ${DAOS_POOL}:${DAOS_CONT}


export IOR_HINT__MPI__cb_config_list="*:8"
export IOR_HINT__MPI__romio_cb_write="enable"
export IOR_HINT__MPI__romio_cb_read="enable"
export LD_PRELOAD=${DARSHAN_RUNTIME_ROOT}/lib/libdarshan.so
mpiexec -n ${NTOTRANKS} --ppn ${NRANKS} --no-vni --envall -cpu-bind ${binding} \
  ior --mpiio.showHints --ncmpi.showHints -c -a NCMPI \
       -b 1000000 -t 1000000 -o daos:/tmp/${DAOS_POOL}/${DAOS_CONT}/ior-pnetcdf.out
