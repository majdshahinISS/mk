PKGDIR	= .
L4DIR	?= $(PKGDIR)/../..

TARGET = myapp2
SRC_CC  = main.cc Test.cc test_log.cc test_pthread.cc test_l4_thread_ipc_1.cc test_dummy.cc test_memory_allocation_free.cc


REQUIRES_LIBS         = libio-io libstdc++ cxx_libc_io cxx_io #ll4re ll4re-util ll4re-c
# this inclusde in the over path of pkg directories where no code
#include $(L4DIR)/mk/subdir.mk 

# this include in underneeth the path of pkg directories where code exists
include $(L4DIR)/mk/prog.mk
