#include "../include/GPIO_wrapper.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <l4/re/error_helper>
#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/io/io.h>

#include <pthread.h>
#include <unistd.h>
#include <cstdint>
#include <limits.h>
#include "../include/log_wrapper.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <l4/sys/l4int.h>
#include "../include/MS_Types.h"
#ifdef __cplusplus
extern "C" {
#endif

////////////////////
#define PRINT_INVALED_PARAMETERS  do{ \
    l4re_log_print(__FUNCTION__);l4re_log_print(": Invalid parameters\n"); \
}while(0)

#define PRINT_NOT_INITIALIZED  do{ \
    l4re_log_print( __FUNCTION__);l4re_log_print(": Object not initialized\n"); \
}while(0)

#define PRINT_ALREADY_INITIALIZED  do{ \
    l4re_log_print( __FUNCTION__);l4re_log_print(": object is already initialized\n"); \
}while(0)

#define PRINT_INVALID_OBJECT  do{ \
    l4re_log_print( __FUNCTION__);l4re_log_print(": Invalid GPIO device object\n"); \
}while(0)

///////////////////

struct GPIO_Resource
{
    unsigned long mem_start= 0;
    unsigned long mem_end= 0;
    unsigned long irq_start = 0;
    unsigned long irq_end = 0;
    volatile bool initialized = false;
};

// TODO : input & output operation must be fit the (channel , directionmask) to not hurt the HW
struct GPIO_Device
{
    uint32_t OBJ_TYPE = OBJ_TYPE_GPIO_Device; // 'GPO '
    char name[GPIO_NAME_MAX_LEN] = {0};
    GPIO_Resource res;
    uint8_t *physaddr;
	uint8_t *virtaddr;
	size_t len;
    //uint32_t channel;
    //uint32_t directionmask;
    volatile bool initialized = false;
};


////////////////////
bool GPIO_Resource_init(const char* device_name, GPIO_Resource *res_out);

////////////////////


bool GPIO_Resource_init(const char* device_name, GPIO_Resource *res_out)
{
    if(res_out->initialized == true)
    {
        PRINT_ALREADY_INITIALIZED;
        return true;
    }
    if ((device_name == nullptr)|| (res_out == nullptr))
    {
        PRINT_INVALED_PARAMETERS;
        return false;
    }
    l4io_device_handle_t dh = l4io_get_root_device();
    l4io_device_t dev;
    l4io_resource_handle_t reshandle;
    long ret=0;
    while((ret=l4io_iterate_devices(&dh, &dev, &reshandle))==0)
    {
        if(strcmp(dev.name,device_name)==0) // device found
        {
            l4io_resource_t mem_res;
            l4io_resource_t irq_res = {UINT16_MAX, UINT16_MAX, ULONG_MAX, ULONG_MAX, LONG_MAX, UINT32_MAX};
            
            // get memory resource
            l4io_resource_handle_t reshandle_save = reshandle;
            long retr=l4io_lookup_resource(dh, L4IO_RESOURCE_MEM, &reshandle, &mem_res);
            if(retr!=0)
            {
                l4re_log_printf("l4io_lookup_resource(L4IO_RESOURCE_MEM) returns %l\n", retr);
                return false;
            }
            // get irq resource
            reshandle =  reshandle_save;
            retr=l4io_lookup_resource(dh, L4IO_RESOURCE_IRQ, &reshandle, &irq_res);
            if(retr!=0)
            {
                l4re_log_printf("l4io_lookup_resource(L4IO_RESOURCE_IRQ) returns %l\n" , retr);
                // return false;
            }
            res_out->mem_start = mem_res.start;
            res_out->mem_end   = mem_res.end;
            res_out->irq_start = irq_res.start;
            res_out->irq_end   = irq_res.end;
            res_out->initialized = true;
            return true; // success
        }
    }
    return false; // device not found
}
bool GPIO_Device_init(void * device_);
bool GPIO_Device_create( const char* device_name, void ** dev)
{
    void *mem = calloc(1,sizeof(GPIO_Device));
    if(mem == nullptr)
    {
        l4re_log_printf("Failed to allocate GPIO_Device for: %s\n", device_name );
        return false;
    }
    GPIO_Device * device = (GPIO_Device*)mem;
    device->OBJ_TYPE = OBJ_TYPE_GPIO_Device;
    int written = snprintf((char*)device->name,
                        GPIO_NAME_MAX_LEN,
                        "%s",
                        device_name);

    if (written < 0) {
        l4re_log_printf("Failed to copy GPIO name\n");
        return false;
    }
    else if (written >= GPIO_NAME_MAX_LEN) {
        l4re_log_printf("GPIO device name too long, truncated: %s\n", device_name);
    }
    
    if(GPIO_Device_init((void*)device) == false)
    {
        l4re_log_printf( "Cannot initialize GPIO_Device: %s\n" ,device_name);
        free(mem);
        return false;
    }
    *dev = mem;
    return true;
}

bool GPIO_Device_check_intern(void * device_, const char * fn)
{
    if (device_ == nullptr)
    {
        l4re_log_printf("%s: Invalid parameters\n",fn);
        return false;
    }
    GPIO_Device *device = (GPIO_Device *)device_;
    
    if (device->OBJ_TYPE != OBJ_TYPE_GPIO_Device)
    {
        l4re_log_printf("%s: Invalid GPIO device object\n",fn);
        return false;
    }
    if (device->initialized == false)
    {
        l4re_log_printf("%s: Object not initialized\n",fn);
        return false;
    }
    return true;
}

bool GPIO_Device_check(void * device_)
{
    if (device_ == nullptr)
        return false;
    
    GPIO_Device *device = (GPIO_Device *)device_;
    
    if (device->OBJ_TYPE != OBJ_TYPE_GPIO_Device)
        return false;
    if (device->initialized == false)
        return false;
    
    return true;
}


bool _In32(void * device_,long offset, uint32_t* res)
{
    // intern function no need to check
    GPIO_Device *device = (GPIO_Device *)device_;
    if(res == nullptr)
    {
        PRINT_INVALED_PARAMETERS;
        return false;
    }

    volatile uint32_t *mem = (volatile uint32_t *)(device->virtaddr + offset);
	uint32_t val = 0;
#ifdef __aarch64__
        // from L4RE l4/pkg/drivers-frst/include/ARCH-arm64/asm_access.h
        asm volatile ("ldr %w[val], %[mem]" : [val] "=r" (val) : [mem] "m" (*mem));
#else
#error Architecture not supported
#endif
	    
	*res = val;
	return true;
}

bool _Out32(void * device_ , long offset, uint32_t val)
{
    // intern function no need to check
    GPIO_Device *device = (GPIO_Device *)device_;
    volatile uint32_t *mem = (volatile uint32_t *)(device->virtaddr+offset);
    // if(_In32(device_, offset, &oldval)== false)
    //    return false;
        
#ifdef __aarch64__
        // from L4RE l4/pkg/drivers-frst/include/ARCH-arm64/asm_access.h
        asm volatile ("str %w[val], %[mem]" : [mem] "=m" (*mem) : [val] "r" (val));
#else
#error Architecture not supported
#endif
	return true;
}

bool GPIO_Device_setdirection(void * device_, uint32_t channel, uint32_t directionmask)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__)==false)
        return false;

    const uint8_t XGPIO_CHAN_OFFSET=8;
    const uint32_t XGPIO_TRI_OFFSET=0x4;

   return _Out32(device_,((channel - 1) * XGPIO_CHAN_OFFSET) + XGPIO_TRI_OFFSET, directionmask);
}


#include <l4/io/io.h>
bool GPIO_Device_init(void * device_)
{
    if (device_ == nullptr)
    {
        PRINT_INVALED_PARAMETERS;
        return false;
    }
        
    GPIO_Device *device = (GPIO_Device *)device_;
    
    if (device->OBJ_TYPE != OBJ_TYPE_GPIO_Device)
    {
        PRINT_INVALID_OBJECT;
        return false;
    }

    if (device->initialized == true)
    {
        l4re_log_printf("%s GPIO device already initialized: %s\n",__FUNCTION__,device->name);
        return true;
    }

    if (!GPIO_Resource_init(device->name, &device->res))
    {
        l4re_log_printf("Failed to get GPIO resources for device: %s\n",device->name);
        return false;
    }

    device->len = device->res.mem_end - device->res.mem_start + 1;
    l4_addr_t virtaddr = 0;
    long ret = l4io_request_iomem(device->res.mem_start, device->len, L4IO_MEM_NONCACHED, &virtaddr );
    if (ret !=0 )
    {
            return false; 
    }
    device->physaddr = (uint8_t *)device->res.mem_start;
    device->virtaddr = (uint8_t *)virtaddr;
    device->initialized = true;

    
    return true;
}

void GPIO_Device_destroy(void** device)
{
    if (*device == nullptr)
    {
        PRINT_INVALED_PARAMETERS;
        return;
    }
    GPIO_Device *dev = (GPIO_Device *)(*device);
    if (dev->OBJ_TYPE != OBJ_TYPE_GPIO_Device)
    {
        PRINT_INVALID_OBJECT;
        return;
    }
    delete (GPIO_Device*)(*device);
    *device = nullptr;
}

void GPIO_Device_show_info(void* device_)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__)==false)
        return;
    GPIO_Device *device = (GPIO_Device *)device_;
    
    l4re_log_printf("GPIO Device Info:\n");
    l4re_log_printf(" Name: %s\n", device->name);
    l4re_log_printf(" Memory Resource: 0x%lx - 0x%lx\n" ,device->res.mem_start , device->res.mem_end);
    l4re_log_printf(" IRQ Resource: 0x%lx - 0x%lx\n", 
              device->res.irq_start ,
              device->res.irq_end );
    l4re_log_printf(" physaddr: 0x%lx , VirtualAddress: 0x%lx, length: 0x%lx\n" ,
        (unsigned long)device->physaddr ,
              (unsigned long)device->virtaddr,
              (unsigned long)device->len);
    l4re_log_printf(" Initialized: %s\n" , (device->initialized ? "Yes" : "No") );
}

bool GPIO_Device_DiscreteWrite(void * device_, unsigned channel, uint32_t data)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__) == false)
        return false;
    if(channel == 0)
    {
        l4re_log_printf("channel num must start with 1\n");
        return false;
    }
    // GPIO_Device *device = (GPIO_Device *)device_;
    return _Out32(device_,(channel-1)*8 + 0x0,data);
}

bool GPIO_Device_DiscretRead(void * device_, unsigned channel, uint32_t * data)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__) == false)
        return false;
    if(channel == 0)
    {
        l4re_log_printf("channel num must start with 1\n");
        return false;
    }
    // GPIO_Device *device = (GPIO_Device *)device_;
    return _In32(device_, (channel-1)*8 + 0x0, data);
}

bool GPIO_Device_InterruptEnable(void * device_, uint32_t mask)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__) == false)
        return false;
    const uint32_t GPIO_IER_OFFSET=0x128;
    uint32_t reg = 0;
    if(!_In32(device_,GPIO_IER_OFFSET, &reg))
    {
        l4re_log_printf( "GPIO_Device_InterruptEnable cannot get interrupt register\n");
        return false;
    }
    return _Out32(device_,GPIO_IER_OFFSET,reg|mask);
}

bool GPIO_Device_InterruptDisable(void * device_, uint32_t mask)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__) == false)
        return false;
    const uint32_t GPIO_IER_OFFSET=0x128;
    uint32_t reg = 0;
    if(!_In32(device_,GPIO_IER_OFFSET, &reg))
    {
        l4re_log_printf( "GPIO_Device_InterruptEnable cannot get interrupt register\n");
        return false;
    }
    return _Out32(device_,GPIO_IER_OFFSET,reg & (~mask));
}

bool GPIO_Device_InterruptClear(void * device_, uint32_t mask)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__) == false)
        return false;
    const uint32_t GPIO_ISR_OFFSET=0x120; // Interrupt status Register
    uint32_t reg = 0;
    if(!_In32(device_,GPIO_ISR_OFFSET, &reg))
    {
        l4re_log_printf("GPIO_Device_InterruptEnable cannot get interrupt register\n");
        return false;
    }
    return _Out32(device_,GPIO_ISR_OFFSET,reg & mask);
}
bool GPIO_Device_GlobalInterruptEnable(void * device_)
{
    if(GPIO_Device_check_intern(device_, __FUNCTION__) == false)
        return false;
    const uint32_t GPIO_GIE_OFFSET=0x11C;
    const uint32_t GPIO_GIE_GINTR_ENABLE_MASK=0x80000000;
    return _Out32(device_,GPIO_GIE_OFFSET,GPIO_GIE_GINTR_ENABLE_MASK);
}
#ifdef __cplusplus
}
#endif