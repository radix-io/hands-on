#!/usr/bin/env bash
#SBATCH -N 8
#SBATCH -n 128
#SBATCH -C haswell
#SBATCH -t 30:00
#SBATCH --qos debug

# Make sure scratch dir exists, is empty, and has default striping
OUTPUT_DIR="$SCRATCH/ior-alignment-demo.$RANDOM"
if [ -d "$OUTPUT_DIR" ]; then
    rmdir -v $OUTPUT_DIR
fi
mkdir -vp $OUTPUT_DIR

# Optional: set striping on the output directory
lfs setstripe -c 4 $OUTPUT_DIR

segmentcount=64
blocksize="1M"
transfersize="1M"

srun -N 8 -n $((8*16)) -C haswell ./ior \
    -s $segmentcount \
    -b $blocksize \
    -t $transfersize \
    -o $OUTPUT_DIR/tempFile.hdf5 \
    -f hdf5.ior | tee unaligned_${transfersize}.out

srun -N 8 -n $((8*16)) -C haswell ./ior \
    -s $segmentcount \
    -b $blocksize \
    -t $transfersize \
    -O setAlignment=1M \
    -o $OUTPUT_DIR/tempFile.hdf5 \
    -f hdf5.ior | tee aligned_${transfersize}.out
