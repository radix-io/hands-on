#!/bin/bash
#SBATCH -N 4
#SBATCH -n 64
#SBATCH -t 30:00
#SBATCH -C haswell
#SBATCH --qos debug

# make sure scratch dir exists, is empty, and has default striping
OUTPUT_DIR="$SCRATCH/ior-cache-demo.$RANDOM"
if [ -d "$OUTPUT_DIR" ]; then
    rmdir -v $OUTPUT_DIR
fi
mkdir -vp $OUTPUT_DIR

# Basic IOR options to be used for all tests
#  -g: accurately distinguishes time spent doing I/O from metadata
#  -i 4: run each test four times
#  -v: be verbose (report open/close time separately from read/write time)
#  -o: location to perform I/O tests
BASE_OPTS="-g -i4 -v -o $OUTPUT_DIR/testFile"

# Basic I/O pattern for all tests
#  -t 1m: transfer 1 MiB at a time to write out a block
#  -b 16m: each rank writes out 16 MiB in a contiguous segment
#  -s 16: each rank writes out 26 strided segments
NAIVE_OPTS="$BASE_OPTS -t 1m -b 16m -s 16"

# submit jobs
echo "Running with naive options"
srun ./ior $NAIVE_OPTS  > cache_demo.out.0 2>&1
grep ^Max cache_demo.out.0

echo "Running with -F"
srun ./ior $NAIVE_OPTS -F > cache_demo.out.1 2>&1
grep ^Max cache_demo.out.1

echo "Running with -F -C"
srun ./ior $NAIVE_OPTS -F -C > cache_demo.out.2 2>&1
grep ^Max cache_demo.out.2

echo "Running with -F -C -e"
srun ./ior $NAIVE_OPTS -F -C -e > cache_demo.out.3 2>&1
grep ^Max cache_demo.out.3

echo "Running with -C -e"
srun ./ior $NAIVE_OPTS -C -e > cache_demo.out.4 2>&1
grep ^Max cache_demo.out.4

lfs setstripe -c 8 $OUTPUT_DIR
echo "Running with -C -e, this time with striping"
srun ./ior $NAIVE_OPTS -C -e > cache_demo.out.5 2>&1
grep ^Max cache_demo.out.5

# Clean up
rmdir -v $OUTPUT_DIR
