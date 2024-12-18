#! /bin/bash

# $1= directory
# $2 = number of laplacian threads
# $3 = output file name for results


gcc -D LAPLACIAN_THREADS=$2 edge_detector.c -o edge_detector;
for FILE in $1/*.ppm; do
		total=0;
		count=0;
		avg=0;

		for ((i=1; i<=20; i++)); do
			((count++))
			temp=$(./edge_detector "$FILE")
			total=$(echo "scale=4;$temp+$total" | bc)
			echo "file: "$FILE" file size: $(wc -c < $FILE) threads: $2, execution #$count"
		done

		avg=$(echo "scale=4;$total/$count"|bc)

		# Number of threads, file name, File size, Average Runtime (seconds)
		echo "$2, "$FILE", $(wc -c < "$FILE"), $avg" >> $3
done
