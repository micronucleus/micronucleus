
include $(SRCDIR)/hardware/t85-2.mk

DEFINES += -DBUILD_JUMPER_MODE -DSTART_JUMPER_PORT=B -DSTART_JUMPER_PIN=5
DEFINES += -DKEEP_OSCCAL
