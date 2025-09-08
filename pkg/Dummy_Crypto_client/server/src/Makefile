PKGDIR   ?= ../..
L4DIR    ?= $(PKGDIR)/../..

TARGET	:= Dummy_Crypto_client
SRC_CC	:= main.cc 


REQUIRES_LIBS         = libio-io libstdc++ cxx_libc_io cxx_io
# this inclusde in the over path of pkg directories where no code
#include $(L4DIR)/mk/subdir.mk 

# this include in underneeth the path of pkg directories where code exists
include $(L4DIR)/mk/prog.mk