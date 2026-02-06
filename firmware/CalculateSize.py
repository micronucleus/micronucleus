#!/usr/bin/python3
# calculate bootloader location and free userspace

import sys
import math

def printmnsize(codesize,pagesize=64,memsize=8192):
    pages=math.ceil(codesize/pagesize)
    freespace=memsize-pages*pagesize-6 # 6 is POSTSCRIPT_SIZE
    bootstart=memsize-pages*pagesize

    print('Codesize: {:04d} bytes, BOOTLOADER_ADDRESS: 0x{:04X}, Free user memory {:04d} bytes.'.format(codesize,bootstart,freespace))

codesize=0

for currentline in sys.stdin.readlines():
    splitline=currentline.split()
    try:
        if splitline[0] == 'Total':
            codesize = int(splitline[1])
    except:
        pass

if (codesize == 0):
    print("Codesize could not be found!")
    exit()

print('8K  Device with 64 byte pages -- ',end='')
printmnsize(codesize,pagesize=64,memsize=8192)
print('16K Device with 128 byte pages -- ',end='')
printmnsize(codesize,pagesize=128,memsize=16384)
print('32K Device with 128 byte pages -- ',end='')
printmnsize(codesize,pagesize=128,memsize=32768)
print('The above values are as a reference only.')
print('Please check the datasheet of your microcontroller')
print('and verify the page and flash sizes.')
