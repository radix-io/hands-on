# modules needed for ATPESC 2025 I/O examples on aurora.alcf.anl.gov
module add parallel-netcdf
module add hdf5
module add frameworks
# we are using a custom Darshan install for ATPESC2025; the darshan module
# availale in the default module path as of July 25, 2025 has an unusual
# configuration and does not auto-instrument applications
module use /flare/ATPESC2025/track7-io/modulefiles/
module load darshan-runtime/3.4.7

# point to manual PyDarshan install in ATPESC scratch space
export PYTHONPATH="/flare/ATPESC2025/track7-io/soft/pydarshan-3.4.7-patched/lib/python3.10/site-packages/:$PYTHONPATH"
