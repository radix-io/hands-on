# modules needed for ATPESC 2025 I/O examples on polaris.alcf.anl.gov
module add cray-parallel-netcdf
module add cray-hdf5-parallel
module add cray-netcdf-hdf5parallel
module add darshan
module add cray-python

# point to manual PyDarshan install in ATPESC scratch space
export PYTHONPATH="/eagle/ATPESC2025/track7-io/soft/pydarsan-3.4.7/lib/python3.11/site-packages/:$PYTHONPATH"
