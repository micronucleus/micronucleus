#!/bin/sh

for bootloader in $(ls ../firmware/releases); do 
	python3 generate-data.py ../firmware/releases/${bootloader}
	make clean
	make
	mv upgrade.hex releases/upgrade-${bootloader}
done
make clean
