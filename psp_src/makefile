TARGET = wifitest
APPOBJS= main.o pg_redux.o nlh.o util.o loadutil.o stubs.o


OBJS = $(APPOBJS) 

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS) -c

LIBDIR =
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Remote Canvas 0.6

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak