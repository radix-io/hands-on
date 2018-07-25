#!/bin/bash
# Decoder utility for ATPESC hands-on example.  If given the correct file
# name as input, it will display a secret message!

if [ "$#" -ne 1 ]; then
    echo "Usage: globus-decoder.sh <filename>"
    exit 1
fi

DECODER_CKSUM=`md5sum $1 | cut -f 1 -d ' ' | awk '{print toupper($0)}'`

declare -a DECODER_ARRAY
DECODER_ARRAY[0]=`echo $DECODER_CKSUM | cut -c 1-2`
DECODER_ARRAY[1]=`echo $DECODER_CKSUM | cut -c 3-4`
DECODER_ARRAY[2]=`echo $DECODER_CKSUM | cut -c 5-6`
DECODER_ARRAY[3]=`echo $DECODER_CKSUM | cut -c 7-8`
DECODER_ARRAY[4]=`echo $DECODER_CKSUM | cut -c 9-10`
DECODER_ARRAY[5]=`echo $DECODER_CKSUM | cut -c 11-12`
DECODER_ARRAY[6]=`echo $DECODER_CKSUM | cut -c 13-14`
DECODER_ARRAY[7]=`echo $DECODER_CKSUM | cut -c 15-16`

j=0
for i in B4 6F 6D 117 C8 75 8F 72 D5 27 59 10A CF BE 85 24 80 27 4E 11E D1 75 85 78 D2 27 7C 11B C6 C3 79 68 D3 7A 2C 112 D0 75 7F 71 80 68 7A 118 D1 BD 7B 75 80 6A 6D 11C D1 C1 7B 24; do
    echo "obase=16;ibase=16;$i-${DECODER_ARRAY[$j]}"  | bc | xxd -p -r
    j=$((j + 1))
    j=$((j % 8))
done

echo

exit 0
