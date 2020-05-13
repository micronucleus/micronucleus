#!/bin/sh

mkdir -p releases
mkdir -p upgrades

for config in $(ls configuration) ; do 
	make clean
	CONFIG=$config make
	mv main.hex releases/${config}.hex
	mv upgrade.hex upgrades/upgrade-${config}.hex
done

make clean
