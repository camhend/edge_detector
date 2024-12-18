#! /bin/bash

# $1= directory containing ppm files
# $2 = number of laplacian threads
# $3 = output file name for results

total=0;
count=0;
avg=0;

gcc -D LAPLACIAN_THREADS=$2 edge_detector.c -o edge_detector;
for ((i=1; i<=50; i++)); do
	((count++))
	temp=$(./run_program.sh "$1")
	total=$(echo "scale=4;$temp+$total" | bc)
	echo "threads: $2, execution #$count"
done

avg=$(echo "scale=4;$total/$count"|bc)

# Number of threads, Num threads, Cores, Average Runtime (seconds)
echo "$2, $count, $(nproc), $avg" >> $3
