#!/bin/sh

mkdir -p releases_
mkdir -p upgrades_

for config in $(ls configuration) ; do 
	make clean
	CONFIG=$config make
	mv main.hex releases_/${config}.hex
	mv upgrade.hex upgrades_/upgrade-${config}.hex
done

make clean
