#ifndef DMAMEM_H
#define DMAMEM_h
#include <l4/re/dataspace>
#include <l4/re/dma_space>
#include <l4/re/rm>
#include <l4/re/env>
#include <l4/re/util/unique_cap>
#include <l4/re/util/cap_alloc>
#include <l4/re/protocols.h>
#include <l4/re/error_helper>

int allocate_dmamem(unsigned long size_in_bytes, unsigned long flags,
		unsigned long phys_align, void **virt_addr,
		L4Re::Util::Unique_cap < L4Re::Dma_space > &dmaspace,
		L4Re::Dma_space::Dma_addr * phys_addr);

int free_dmamem(void *virt_addr);
#endif
