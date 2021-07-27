# modules needed for ATPESC 2021 I/O examples on theta.alcf.anl.gov
module add cray-parallel-netcdf
module add cray-hdf5-parallel
module add cray-netcdf-hdf5parallel
module add texlive

# the 'miniconda' distribution contains mpi4py, numpy, and several other useful
# python modules

module add miniconda-3/latest

# additional environment tweaks:
export PERLLIB=/projects/ATPESC2020/atpesc-io/perl5
