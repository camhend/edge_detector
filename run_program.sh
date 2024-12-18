#! /bin/bash

args=()

for filename in "$1/*.ppm"; do
	args+=" $filename"
done

./edge_detector $args


