PKGDIR ?= .
L4DIR  ?= $(PKGDIR)/../..

TARGET = libl4re_cppwrap.a
SRC_CC = src/log_wrapper.cc src/registry_server_wrap.cc
HDRS   = include/log_wrapper.h include/registry_server_wrap.h

REQUIRES_LIBS = libstdc++
include $(L4DIR)/mk/lib.mk
