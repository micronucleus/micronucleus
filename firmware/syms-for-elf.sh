#!/bin/bash

input=$1

avr-nm --format=posix $input | while read name type addr
do
	case "$name" in
	__vector_*)
		;;
	__init)
		;;
	*)
		continue
	esac
	echo "app${name} = 0x$addr;"
done
