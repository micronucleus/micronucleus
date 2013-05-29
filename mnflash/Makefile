
ENABLE_SHARED=

DESTDIR=

prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
includedir=$(prefix)/include
man1dir=$(prefix)/share/man/man1

LIBS=\
	-lusb \
	-lelf \
	-lm

CFLAGS=\
	-Wall \
	-g \
	-I include \
	-O2

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

LIBBASE=libmnflash
#----- static lib stuff -----
LIBOBJS=$(LIBSOURCE:%.c=%.o)
LIBNAME=$(LIBBASE).a

#----- shared lib stuff -----
SOMAJOR=0
SOMINOR=1
SHLIBNAME=$(LIBBASE).so.$(SOMAJOR).$(SOMINOR)
SONAME=$(LIBBASE).so.$(SOMAJOR)
LNNAME=$(LIBBASE).so

SHLIBOBJS=$(LIBSOURCE:%.c=%.po)
LDSHAREDFLAGS=-shared -Wl,-soname,$(SONAME)

#----- all libs -----
BUILDLIBS := $(LIBNAME)

ifdef ENABLE_SHARED
BUILDLIBS += $(SHLIBNAME)
LIBMNFLASH= -L. -lmnflash
else
LIBMNFLASH= $(LIBNAME)
endif

#------ recipes -----

Q=@

all:	$(BUILDLIBS)  mnflash

Makefile.dep:	$(SOURCE)
	@echo "GEN $@ ..."
	$(Q)$(CC) -MM $(CFLAGS) $(SOURCE) $(LIBSOURCE) > $@

include Makefile.dep

.c.o:
	@echo "CC  $@ ..."
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.po: %.c
	@echo "CC  $@ ... (pic)"
	$(Q)$(CC) $(CFLAGS) -fpic -c $< -o $@

$(LIBNAME):	$(LIBOBJS)
	@echo "AR  $@ ..."
	$(Q)$(AR) rcs $@ $(LIBOBJS)

$(SHLIBNAME):	$(SHLIBOBJS)
	@echo "LD  $@ (shared) ..."
	$(Q)$(CC) $(LDSHAREDFLAGS) -o $@ $(SHLIBOBJS)
	$(Q)ln -s $@ $(SONAME)
	$(Q)ln -s $(SONAME) $(LNNAME)

mnflash:	$(OBJS) libmnflash.a
	@echo "LD  $@ ..."
	$(CC) $(CFLAGS) $(OBJS) $(LIBMNFLASH) $(LIBS) -o $@

mnflash.pc:	mnflash.pc.in
	sed -e 's#@@prefix@@#$(prefix)#g' \
	    -e 's#@@libdir@@#$(libdir)#g' \
	    -e 's#@@includedir@@#$(includedir)#g' \
	    <$< >$@

#----- install stuff ------
destdirs= \
	$(DESTDIR)$(bindir) \
	$(DESTDIR)$(libdir) \
	$(DESTDIR)$(libdir)/pkgconfig \
	$(DESTDIR)$(includedir)/libmnflash \
	$(DESTDIR)$(man1dir) \

destdirs:
	for x in $(destdirs); do mkdir -p $$x; done

install-libs: $(BUILDLIBS) destdirs mnflash.pc
	install -m644 $(LIBNAME) $(DESTDIR)$(libdir)
	install -m644 include/libmnflash/*.h $(DESTDIR)$(includedir)/libmnflash
	install -m644 mnflash.pc $(DESTDIR)$(libdir)/pkgconfig
ifdef ENABLE_SHARED
	install -m755 $(SHLIBNAME) $(DESTDIR)$(libdir)
	(cd $(DESTDIR)$(libdir) && \
	 rm -f $(SONAME) && ln -s $(SHLIBNAME) $(SONAME) && \
	 rm -f $(LNNAME) && ln -s $(SONAME) $(LNNAME))
ifndef DESTDIR
	test -x /sbin/ldconfig && /sbin/ldconfig
endif
endif

install-bin: mnflash destdirs
	install -m755 mnflash $(DESTDIR)$(bindir)

install: all destdirs install-libs install-bin


clean:
	$(MAKE) -C example clean
	rm -f Makefile.dep
	rm -f *.o lib/*.o lib/*.po
	rm -f libmnflash.a
	rm -f $(SHLIBNAME) $(SONAME) $(LNNAME)
	rm -f mnflash.pc
	rm -f mnflash

.PHONY:	all clean destdirs intall install-libs
