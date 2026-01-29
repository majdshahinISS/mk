PKGDIR   ?= ../..
L4DIR    ?= $(PKGDIR)/../..

TARGET := Dummy_Crypto_server
SRC_CC  := server.cc 

CXXFLAGS += -I$(L4DIR)/pkg/DataspaceEndpoint

REQUIRES_LIBS         = libdataspaceendpoint libio-io libstdc++ cxx_libc_io cxx_io l4re l4re-util libpthread
# this inclusde in the over path of pkg directories where no code
#include $(L4DIR)/mk/subdir.mk 

# this include in underneeth the path of pkg directories where code exists
include $(L4DIR)/mk/prog.mk
