

LIBS=\
	-lusb \
	-lelf \
	-lm

CFLAGS=\
	-Wall \
	-g \
	-I include \
	-O2

all:	mnflash

SOURCE= \
	mnflash.c

LIBSOURCE= \
	lib/usb-device.c \
	lib/firmware.c \
	lib/firmware-blob.c \
	lib/load-elf.c \
	lib/load-ihex.c \
	lib/load-raw.c \
	lib/hexdump.c \
	lib/uploader.c

OBJS=$(SOURCE:%.c=%.o)
LIBOBJS=$(LIBSOURCE:%.c=%.o)

Q=@

Makefile.dep:	$(SOURCE)
	@echo "GEN $@ ..."
	$(Q)$(CC) -MM $(CFLAGS) $(SOURCE) $(LIBSOURCE) > $@

include Makefile.dep


.c.o:
	@echo "CC  $@ ..."
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

libmnflash.a:	$(LIBOBJS)
	@echo "AR  $@ ..."
	$(Q)$(AR) rcs $@ $(LIBOBJS)

mnflash:	$(OBJS) libmnflash.a
	@echo "LD  $@ ..."
	$(Q)$(CC) $(CFLAGS) $(OBJS) libmnflash.a $(LIBS) -o $@

clean:
	rm -f Makefile.dep
	rm -f *.o lib/*.o
	rm -f libmnflash.a
	rm -f mnflash

.PHONY:	all clean
