#!/bin/bash
#SBATCH -N 1
#SBATCH -C gpu
#SBATCH --gpus 4
#SBATCH --gpus-per-node=4
#SBATCH --time 30
#SBATCH -q debug
#SBATCH -A trn015
#SBATCH -o %x-%j.out

# setup software
module load pytorch
module load darshan

export MASTER_ADDR=$(hostname)
export MASTER_PORT=29500

# run the model
time python distributed-checkpoint-vision-vit.py
