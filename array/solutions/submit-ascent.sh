#!/bin/bash

# workaround for bsub not able to take arguments.  Use a here doc to construct
# a shell script on the fly from arguments, then submit the generated script
# thanks Thomas Papatheodore at NCSS for the suggestion

cat > batch.job << EOF

#BSUB -P gen139
#BSUB -J atpesc-io
#BSUB -o %J.out
#BSUB -e %J.err
#BSUB -nnodes 1
#BSUB -W 00:10

# '.' in path not best practice but will save a few headaches
export PATH=.:${PATH}

APPLICATION=$1

TRAINING_DIR=/ccsopen/proj/gen139/data-and-io/$USER

# shell expansion syntax: if there is no second argument, use the file name
# 'testfile' as default value
FILENAME=${2:-testfile}


eval echo "writing to \${TRAINING_DIR}/\${FILENAME}"
eval jsrun -r 1 -c ALL_CPUS -a 32  \$APPLICATION \${TRAINING_DIR}/\${FILENAME}

EOF

bsub batch.job

rm batch.job
