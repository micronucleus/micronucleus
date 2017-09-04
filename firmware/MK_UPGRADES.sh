#!/bin/sh

for config in $(ls configuration) ; do 
	make clean
	CONFIG=$config make
	mv upgrade.hex upgrades/upgrade-${config}.hex
done
make clean
