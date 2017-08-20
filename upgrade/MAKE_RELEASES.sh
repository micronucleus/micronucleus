#!/bin/sh

for bootloader in $(ls ../firmware/releases); do 
	make clean
	ruby generate-data.rb ../firmware/releases/${bootloader}
	make
	mv upgrade.hex releases/upgrade-${bootloader}
	make clean
done
