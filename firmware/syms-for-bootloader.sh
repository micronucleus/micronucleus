#!/bin/bash

input=$1

avr-nm --format=posix $input | while read name type addr size
do
	case "$name" in
	__my_reset2)
		echo "app__init = 0x$addr;"
		;;
	__wrap_vusb_intr)
		interrupt_addr=$addr
		for x in `seq 1 127`; do
			echo "app__vector_$x = 0x$addr;"
		done
		;;
	*)
		continue
	esac
done


