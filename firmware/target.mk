.SUFFIXES:

ifndef HARDWARE
OBJDIRS := $(patsubst %,target-%,$(ALL_HARDWARE))
else
OBJDIRS := target-$(HARDWARE)
endif

MAKETARGET = $(MAKE) --no-print-directory -C $@ HARDWARE=$(patsubst target-%,%,$@) -f $(CURDIR)/Makefile \
	SRCDIR=$(CURDIR) $(MAKECMDGOALS)

all-targets: $(OBJDIRS)
	./sizes.sh

install: $(OBJDIRS)

.PHONY: $(OBJDIRS)

$(OBJDIRS):
	+@[ -d $@ ] || mkdir -p $@
	+@[ -d $@/usbdrv ] || mkdir -p $@/usbdrv
	+@[ -d $@/libs-device ] || mkdir -p $@/libs-device
	+@echo "######################## $@ ##########################"
	+@$(MAKETARGET)


Makefile : ;
%.mk :: ;
% :: $(OBJDIR) ; :

.PHONY: clean

clean:
	rm -rf $(OBJDIRS)

