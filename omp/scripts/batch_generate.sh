#/usr/bin/env bash
# generate jobs in batch

threads=(1) # The number of threads 
inputs=(medium_16x16.txt) # The name of the input files
rm -f *.job

for f in ${inputs[@]}
do
    for t in ${threads[@]}
    do
	../scripts/generate_jobs.sh $f $t
    done
done
