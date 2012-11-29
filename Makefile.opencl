CC=gcc
CFLAGS=-O3 -Wall
CFLAGS+=-DOPENCL
CFLAGS+=-DHAVE_OPENSSL
#CFLAGS+=-DDEBUG
LDFLAGS=-lssl -lOpenCL
SOURCES=sudo.c resolve_cl.c resolve_iterative.c resolve_recursive.c resolve.c main.c
EXEC=makedoku

ROOTDIR    ?= /home/$(USER)/NVIDIA_GPU_Computing_SDK
LIBDIR	   := $(ROOTDIR)/shared/lib
SHAREDDIR  := $(ROOTDIR)/shared
OCLROOTDIR := $(ROOTDIR)/OpenCL
OCLCOMMONDIR ?= $(OCLROOTDIR)/common
OCLBINDIR    ?= $(OCLROOTDIR)/bin
BINDIR	     ?= $(OCLBINDIR)/$(OSLOWER)
OCLLIBDIR    := $(OCLCOMMONDIR)/lib
INCDIR	     ?= .
INCLUDES  += -I$(INCDIR) -I$(OCLCOMMONDIR)/inc -I$(SHAREDDIR)/inc

all:
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
