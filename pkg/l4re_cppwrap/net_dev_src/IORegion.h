#ifndef IOREGION_H
#define IOREGION_H
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <l4/sys/l4int.h>

/**
 * class IORegion
 *
 * maps an Region in the IO-memory mapped register space.
 * This class is for bare metal and for L4RE oder other OSs
 */
class IORegion
{
public:
	IORegion(const char *name):physaddr_(0),name_(name),
	                           virtaddr_(0),len_(0)
    {}
	void initmem(uint8_t *physaddr,uint8_t *virtaddr,size_t len);
    void initmem(l4_addr_t phyaddr_start, l4_addr_t phyaddr_end);
protected:
	uint32_t In32(off_t offset)
	{
	    volatile uint32_t *mem = (volatile uint32_t *)(virtaddr_+offset);
	    uint32_t val = 0;
#ifdef __aarch64__
        // from L4RE l4/pkg/drivers-frst/include/ARCH-arm64/asm_access.h
        asm volatile ("ldr %w[val], %[mem]" : [val] "=r" (val) : [mem] "m" (*mem));
#else
#error Architecture not supported
#endif
#ifdef DEBUGIO
	    printf("%s:In32(%lx)=%x\n",name_,offset,val);
#endif
	    return val;
	}

	uint32_t In32_silent(off_t offset)
	{
	    volatile uint32_t *mem = (volatile uint32_t *)(virtaddr_+offset);
        uint32_t val = 0;
#ifdef __aarch64__
        // from L4RE l4/pkg/drivers-frst/include/ARCH-arm64/asm_access.h
        asm volatile ("ldr %w[val], %[mem]" : [val] "=r" (val) : [mem] "m" (*mem));
#else
#error Architecture not supported
#endif
	    return val;
	}

	void Out32(off_t offset,uint32_t val)
	{
#ifdef DEBUGIO
	    uint32_t oldval = In32_silent(offset);
	    uint32_t newval=0;
#endif
	    volatile uint32_t *mem = (volatile uint32_t *)(virtaddr_+offset);
#ifdef __aarch64__
        // from L4RE l4/pkg/drivers-frst/include/ARCH-arm64/asm_access.h
        asm volatile ("str %w[val], %[mem]" : [mem] "=m" (*mem) : [val] "r" (val));
#else
#error Architecture not supported
#endif
#ifdef DEBUGIO
	    newval = In32_silent(offset);
	    printf("%s:Out32(%lx):old=%x value=%x new=%x\n",name_,offset,oldval,val,newval);
#endif
	 }
	uint8_t *physaddr_;
private:
	const char *name_;
	uint8_t *virtaddr_;
	size_t len_;
};
#endif
