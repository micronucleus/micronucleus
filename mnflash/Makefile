

LIBS=\
	-lusb \
	-lelf \
	-lm

CFLAGS=\
	-Wall \
	-g 

all:	mnflash

SOURCE= \
	mnflash.c

LIBSOURCE= \
	usb-device.c \
	firmware.c \
	firmware-blob.c \
	load-elf.c \
	load-ihex.c \
	load-raw.c \
	hexdump.c \
	uploader.c

OBJS=$(SOURCE:%.c=%.o)
LIBOBJS=$(LIBSOURCE:%.c=%.o)

Makefile.dep:	$(SOURCE)
	$(CC) -MM $(CFLAGS) $(SOURCE) > $@

include Makefile.dep

libmnflash.a:	$(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)

mnflash:	$(OBJS) libmnflash.a
	$(CC) $(CFLAGS) $(OBJS) libmnflash.a $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f Makefile.dep
	rm -f libmnflash.a
	rm -f mnflash

.PHONY:	all clean
