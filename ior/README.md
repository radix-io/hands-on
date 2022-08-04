# IOR: the HPC IO benchmark

To obtain IOR go to https://github.com/hpc/ior

Noncontiguous IOR requires some bug fixes that are so far only available in git main.

## Experiments:

* ior-simple-theta.sh:  an example of how to do a typical contiguous IOR benchmark
* ior-noncontig-theta.sh: demonstrating how to measure MPI-IO noncontiguous performance, with and without "data sieving" optimization
* ior-noncontig-collective-theta.sh: demonstrating collective IOR and the impact of tuning knobs
