Benchmarking 
================================================================================

This directory contains some examples that illustrate some foundational concepts
in parallel I/O.

Caching Demo - cache-demo.sbatch
--------------------------------------------------------------------------------

This demo uses IOR to demonstrate specific techniques to avoid caching effects,
but you are best off using your own application's I/O pattern.

The example job script, cache-demo.sbatch, is made for NERSC's Cori system, but
converting this to work with Cobalt on Theta is not difficult.  As an exercise
to the reader, give it a try on Theta.

If you just want to see exactly what happens when this is run, consult the
contents of

- cache-demo.out.0/
- cache-demo.out.1/

These both contain the results of two runs of the same test on NERSC's Cori
system.  The slurm output log contains a concise summary of the tests and
their results; the `cache_demo.out.?` files contain the outputs of the IOR
jobs, and the Darshan log files contain the logs corresponding to each step.

To run these tests yourself, you must first build IOR.  The code and
instructions to do that are below.

Alignment Demo - alignment-demo.out.stripe4
--------------------------------------------------------------------------------

This experiment demonstrates the performance difference between a shared-file
write workload that was performed with writes aligned on specific boundaries
(e.g., every write starts on a 1 MiB offset) and writes performed without any
alignment.

Data is generated from a set of IOR runs that use the HDF5 API.  Because HDF5
includes small metadata headers before each dataset, it is prone to performing
slightly misaligned I/Os unless `H5Pset_alignment()` is explicitly used.  Each

There are two sample output directories that you can examine:

- alignment-demo.out.stripe1/ contains the results of writing a single HDF5 file
  from four nodes without any Lustre striping, so all four nodes were writing to
  the same OST.  This maximizes misalignment-induced lock contention.
- alignment-demo.out.stripe4/ contains the results of writing a single HDF5 file
  from four nodes with striping over four OSTs.  While the net performance is
  better than the single-OST case, performance loss due to misalignment is still
  significant.

IOR-3.2.1+atpesc
--------------------------------------------------------------------------------

This is a version of IOR 3.2.1 with a minor modification to address
[issue #24][].

To build on a Cray XC system,

    cd ior-3.2.1+atpesc
    module unload cray-netcdf
    module load cray-hdf5-parallel
    module load cray-parallel-netcdf
    ./configure --with-hdf5=$CRAY_HDF5_DIR --with-ncmpi=$CRAY_PARALLEL_NETCDF_DIR --without-gpfs
    make

This will deposit a binary called `ior` in `ior-3.2.1+atpesc/src/`.  Copy this
to a convenient location (e.g., the directory from which you will be issuing the
sbatch or qsub commands).

[issue #24]: https://github.com/hpc/ior/issues/24
