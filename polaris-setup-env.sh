# modules needed for ATPESC 2023 I/O examples on theta.alcf.anl.gov
module add cray-parallel-netcdf
module add cray-hdf5-parallel
module add cray-netcdf-hdf5parallel
module add darshan

# load a more recent Python version
module add cray-python

# point to manual PyDarshan install in ATPESC scratch space
export PYTHONPATH=/eagle/projects/ATPESC2024/usr/soft/track-7-io/pydarshan:$PYTHONPATH
