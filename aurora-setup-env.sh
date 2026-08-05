# modules needed for ATPESC 2026 I/O examples on aurora.alcf.anl.gov
module add parallel-netcdf
module add hdf5
module load darshan-runtime/3.4.7

# needed newer "darshan utils"  and "py-darshan" to get DAOS support
module load python/3.12.12
export LD_LIBRARY_PATH=/lus/flare/projects/ATPESC_Instructors/track9-io/soft/darshan-master/lib/:${LD_LIBRARY_PATH}
export PYTHONPATH="/lus/flare/projects/ATPESC_Instructors/track9-io/soft/darshan-master/lib/python3.12/site-packages:$PYTHONPATH"

# daos not yet part of the default environment
module use /soft/modulefiles
module load daos
