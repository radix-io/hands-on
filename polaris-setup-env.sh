# modules needed for ATPESC 2023 I/O examples on theta.alcf.anl.gov
module add cray-parallel-netcdf
module add cray-hdf5-parallel
module add cray-netcdf-hdf5parallel

# load darshan module using MODULEPATH for now
export MODULEPATH=/soft/perftools/darshan/darshan-3.4.3/share/craype-2.x/modulefiles/:$MODULEPATH
module add darshan

# load a more recent Python version
module add cray-python
