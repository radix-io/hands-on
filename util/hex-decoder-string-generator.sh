#!/bin/bash
# Given a file and a human-readable phrase, produces a sequence of
# hex numbers that can be pasted into a script like
# ../globus/globus-decoder.sh to reveal the string when the target file
# matches.  

if [ "$#" -ne 2 ]; then
    echo "Usage: inverter.sh <filename> <\"phrase\">"
    exit 1
fi
PHRASE=$2
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
for i in $(seq 1 ${#PHRASE})
do
    LETTER=${PHRASE:i-1:1}
    LETTER=`printf "%x" "'$LETTER" | awk '{print toupper($0)}'`
    LETTER=`echo "obase=16;ibase=16;$LETTER+${DECODER_ARRAY[$j]}"  | bc`
    echo -n "$LETTER "
    j=$((j + 1))
    j=$((j % 8))
done
echo

exit 0

