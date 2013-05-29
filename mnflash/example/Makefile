

LIBS := $(shell pkg-config --libs mnflash)

CFLAGS := \
	-Wall \
	-g \
	-O2 \
	$(shell pkg-config --cflags mnflash)

all:	mnflash-switch

SOURCE= \
	mnflash-switch.c

OBJS=$(SOURCE:%.c=%.o)

Q=@

Makefile.dep:	$(SOURCE)
	@echo "GEN $@ ..."
	$(Q)$(CC) -MM $(CFLAGS) $(SOURCE) > $@

include Makefile.dep


.c.o:
	@echo "CC  $@ ..."
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

mnflash-switch:	$(OBJS)
	@echo "LD  $@ ..."
	$(Q)$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

clean:
	rm -f Makefile.dep
	rm -f *.o
	rm -f mnflash-switch

.PHONY:	all clean
