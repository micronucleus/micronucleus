#!/bin/sh

for bootloader in $(ls ../firmware/releases); do 
	ruby generate-data.rb ../firmware/releases/${bootloader}
	make clean
	make
	mv upgrade.hex releases/upgrade-${bootloader}
done
make clean
