PKGDIR ?= .
L4DIR  ?= $(PKGDIR)/../..

TARGET = libl4re_cppwrap.a
SRC_CC = 	src/dummy_class.cc \
			src/registry_server_wrap.cc \
			src/GPIO_wrapper.cc  \
			src/Interrupt_wrapper.cc \
			net_dev_src/CRLARB.cc		\
			net_dev_src/IORegion.cc		\
			net_dev_src/DevEmacPs.cc	\
			net_dev_src/xemacpsif.cc	\
			net_dev_src/dumputility.cc	\
			net_dev_src/dmamem.cc		\
			src/xemacpsif_wrapper.cc

			#net_dev_src/xemacpsif.cc
SRC_C	=	net_dev_src/xemacps_bdring.c		
# in archive
#	src/log_wrapper.cc include/log_wrapper.h
#	src/MS_Types.cc include/MS_Types.h	
#	include/dummy.h src/dummy.cc
HDRS   = 	include/dummy_class.h \
			include/registry_server_wrap.h \
			include/GPIO_wrapper.h \
			include/Interrupt_wrapper.h \
			net_dev_src/CRLARB.h		\
			net_dev_src/IORegion.h		\
			net_dev_src/xemacps_bdring.h\
			net_dev_src/DevEmacPs.h		\
			net_dev_src/xemacpsif.h	\
			net_dev_src/dumputility.h	\
			net_dev_src/dmamem.h		\
			include/xemacpsif_wrapper.h


			#net_dev_src/xemacpsif.h
			

# intern : EVBuffer.h xemacps_hw.h xemacps_bd.h ethernet.h 

CFLAGS += -I$(PKGDIR)/include -I/media/iss/Arbeit/Vitis_install/2025.1/data/embeddedsw/lib/bsp/standalone_v9_3/src/common
CPPFLAGS += -I$(PKGDIR)/include -I/media/iss/Arbeit/Vitis_install/2025.1/data/embeddedsw/lib/bsp/standalone_v9_3/src/common

REQUIRES_LIBS = libio-io libio libstdc++ cxx_libc_io cxx_io libpthread# libstdc++ cxx_libc_io cxx_io libsupc++
include $(L4DIR)/mk/lib.mk
