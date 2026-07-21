#!/bin/bash
#PBS -A radix-io
#PBS -lselect=1
#PBS -lwalltime=00:30:00
#PBS -k doe
#PBS -lfilesystems=home:daos_user_fs

# setup software
module load frameworks
module load darshan-runtime

cd $PBS_O_WORKDIR

# set TORCH_HOME to somethig besides your home directory or you'll blow up your
# quota
export TORCH_HOME=/lus/flare/projects/radix-io/robl/torch/
export MASTER_ADDR=$(hostname)
export MASTER_PORT=29500

# run the model
time python checkpoint-vision-vit.py
