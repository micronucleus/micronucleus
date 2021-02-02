# Makefile initially writen for Little-Wire by Omer Kilic <omerkilic@gmail.com>
# Later on modified by ihsan Kehribar <ihsan@kehribar.me> for Micronucleus bootloader application.

SHELL := /bin/bash

CC=gcc

ifeq ($(shell uname), Linux)
# If you get the error "usb.h: No such file or directory" you must run "sudo apt-get install libusb-dev"
	USBFLAGS=$(shell libusb-config --cflags)
	USBLIBS= -static $(shell libusb-config --libs)
	EXE_SUFFIX =
	OSFLAG = -D LINUX
else ifeq ($(shell uname), Darwin)
	USBFLAGS=$(shell libusb-config --cflags || libusb-legacy-config --cflags)
	EXE_SUFFIX =
	OSFLAG = -D MAC_OS
	USBLIBS = /usr/local/opt/libusb-compat/lib/libusb.a
	USBLIBS += /usr/local/opt/libusb/lib/libusb-1.0.a
	USBLIBS += -framework CoreFoundation
	USBLIBS += -framework IOKit
	# Uncomment these to create a dual architecture binary:
	# OSFLAG += -arch x86_64 -arch i386
else ifeq ($(shell uname), OpenBSD)
	USBFLAGS=$(shell libusb-config --cflags || libusb-legacy-config --cflags)
	USBLIBS=$(shell libusb-config --libs || libusb-legacy-config --libs)
	EXE_SUFFIX =
	OSFLAG = -D OPENBSD
else ifeq ($(shell uname), FreeBSD)
	USBFLAGS=
	USBLIBS= -lusb
	EXE_SUFFIX =
	OSFLAG = -D OPENBSD
else # Windows
	USBFLAGS =
	USBLIBS = -lusb0
	EXE_SUFFIX = .exe
	OSFLAG = -D WIN
endif

LIBS    = $(USBLIBS)
CFLAGS  = $(USBFLAGS) -Ilibrary -O -g $(OSFLAG)

LWLIBS = micronucleus_lib littleWire_util

.PHONY:	clean library micronucleus

all: micronucleus
	rm -f *.o

library: $(LWLIBS)

%.o: ./library/%.c
	@echo Building library: $<...
	$(CC) $(CFLAGS) -c $<

micronucleus: $(addsuffix .o, $(LWLIBS))
	@echo Building command line tool: $@$(EXE_SUFFIX)...
	$(CC) $(CFLAGS) -o $@$(EXE_SUFFIX) $@.c $^ $(LIBS)

clean:
	rm -f micronucleus *.o *.exe

install: all
	cp micronucleus /usr/local/bin
