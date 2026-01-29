PKGDIR ?= .
L4DIR  ?= $(PKGDIR)/../..

TARGET = libdataspaceendpoint.a
PC_FILENAME = libdataspaceendpoint
SRC_CC = 	DataspaceEndpoint.cc \
		DataspaceOwner.cc \
		LocalMemoryManager.cc \
		helperFunctions.cc

# SRC_C	=

HDRS   = 	DataspaceEndpoint.h \
		DataspaceOwner.h \
		LocalMemoryManager.h \
		helperFunctions.h \
		crypto_shared.h


REQUIRES_LIBS = libio-io libio libstdc++ cxx_libc_io cxx_io libpthread# libstdc++ cxx_libc_io cxx_io libsupc++
include $(L4DIR)/mk/lib.mk
