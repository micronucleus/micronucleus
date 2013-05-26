#!/bin/bash

printf "%-20s  %s\n" hardware size
echo "-----------------------------"

for dir in target-*; do
	rawfile=$dir/bootloader.raw
	hardware=${dir#target-}
	test -f $rawfile || continue
	size=`avr-size --target=binary $rawfile | tail -1 | cut -f 4`

	printf "%-20s  %d\n" $hardware $size
done
