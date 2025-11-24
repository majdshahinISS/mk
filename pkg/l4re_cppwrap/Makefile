PKGDIR ?= .
L4DIR  ?= $(PKGDIR)/../..

TARGET = libl4re_cppwrap.a
SRC_CC = src/log_wrapper.cc src/registry_server_wrap.cc src/dummy.cc src/MS_Types.cc
HDRS   = include/log_wrapper.h include/registry_server_wrap.h include/dummy.h include/MS_Types.h

REQUIRES_LIBS = libio-io 
include $(L4DIR)/mk/lib.mk
