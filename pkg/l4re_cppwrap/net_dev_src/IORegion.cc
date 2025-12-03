#include "IORegion.h"
#include <cstdlib>
#include <iostream>

void IORegion::initmem(uint8_t *physaddr,uint8_t *virtaddr,size_t len)
{
	physaddr_ = physaddr;
	virtaddr_ = virtaddr;
	len_      = len;
	printf("%s: Map %p<-%p len=%lx\n\r",name_,physaddr,virtaddr,len);
}


//L4
#include <l4/io/io.h>
void
IORegion::initmem(l4_addr_t phyaddr_start, l4_addr_t phyaddr_end)
{
  size_t len = phyaddr_end - phyaddr_start + 1;
  l4_addr_t virtaddr = 0;
  printf("%s-MEM: %08lx - %08lx size=%lx\n",name_, phyaddr_start, phyaddr_end, len);
  long ret = l4io_request_iomem(phyaddr_start, len, L4IO_MEM_NONCACHED, &virtaddr);
  if (ret != 0)
  {
    std::clog << "l4io_request_iomem(" << phyaddr_start << ", " << len
              << ") returns " << ret << std::endl;
    exit(1);
  }
  printf("%s: alloced Addr=%lx\n", name_,virtaddr);
  IORegion::initmem((uint8_t *)phyaddr_start,(uint8_t *)virtaddr,len);
}
