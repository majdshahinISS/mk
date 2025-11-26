PKGDIR ?= .
L4DIR  ?= $(PKGDIR)/../..

TARGET = libl4re_cppwrap.a
SRC_CC = 	src/dummy_class.cc \
			src/registry_server_wrap.cc \
			src/GPIO_wrapper.cc  \
			src/Interrupt_wrapper.cc
			
# in archive
#	src/log_wrapper.cc include/log_wrapper.h
#	src/MS_Types.cc include/MS_Types.h	
#	include/dummy.h src/dummy.cc
HDRS   = 	include/dummy_class.h \
			include/registry_server_wrap.h \
			include/GPIO_wrapper.h \
			include/Interrupt_wrapper.h
			

REQUIRES_LIBS = libio-io libio # libstdc++ cxx_libc_io cxx_io libsupc++
include $(L4DIR)/mk/lib.mk
