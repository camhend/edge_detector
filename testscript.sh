#! /bin/bash

count=0;
total=0.5;

for ((i=1 ; i<=10; i++)); do
	total=$(echo "scale=5;($total+5)/2" | bc)
	echo $total
	echo $count
	((count++))
done

echo "total / count"
echo "scale=5;$total / $count" | bc
