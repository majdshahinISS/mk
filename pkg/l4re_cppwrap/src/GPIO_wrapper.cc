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
// #include "../include/log_wrapper.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <l4/sys/l4int.h>
// #include "../include/MS_Types.h"


////////////////////
#define PRINT_INVALED_PARAMETERS  do{ \
    std::cout << __FUNCTION__ << ": Invalid parameters" << std::endl; \
}while(0)

#define PRINT_NOT_INITIALIZED  do{ \
    std::cout << __FUNCTION__ << ": Object not initialized" << std::endl; \
}while(0)

#define PRINT_ALREADY_INITIALIZED  do{ \
    std::cout << __FUNCTION__ << ": object is already initialized" << std::endl; \
}while(0)

#define PRINT_INVALID_OBJECT  do{ \
    std::cout << __FUNCTION__ << ": Invalid GPIO device object" << std::endl; \
}while(0)

///////////////////

class GPIO_Resource {
public:
    GPIO_Resource() : mem_start(0), mem_end(0), irq_start(0), irq_end(0), initialized(false) {}

    bool init(const char* device_name);
    //void deinit();

    unsigned long mem_start;
    unsigned long mem_end;
    unsigned long irq_start;
    unsigned long irq_end;
    volatile bool initialized;
};

bool GPIO_Resource::init(const char* device_name) {
    if (initialized) {
        PRINT_ALREADY_INITIALIZED;
        return true;
    }
    if (!device_name) {
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
                std::cout<<"l4io_lookup_resource(L4IO_RESOURCE_MEM) returns " << retr << std::endl;
                return false;
            }
            // get irq resource
            reshandle =  reshandle_save;
            retr=l4io_lookup_resource(dh, L4IO_RESOURCE_IRQ, &reshandle, &irq_res);
            if(retr!=0)
            {
                std::cout<<"l4io_lookup_resource(L4IO_RESOURCE_IRQ) returns " << retr << std::endl;
                // return false;
            }
            mem_start = mem_res.start;
            mem_end   = mem_res.end;
            irq_start = irq_res.start;
            irq_end   = irq_res.end;
            initialized = true;
            return true; // success
        }
    }
    return false; // device not found
}

class GPIO_Device {
public:
    GPIO_Device(const char* device_name);
    ~GPIO_Device();

    bool init();
    bool check(const char* fn) const;
    bool setDirection(uint32_t channel, uint32_t directionmask);
    bool discreteWrite(unsigned channel, uint32_t data);
    bool discreteRead(unsigned channel, uint32_t* data);
    bool interruptEnable(uint32_t mask);
    bool interruptDisable(uint32_t mask);
    bool interruptClear(uint32_t mask);
    bool globalInterruptEnable();
    void showInfo() const;

private:
    char name[GPIO_NAME_MAX_LEN];
    GPIO_Resource res;
    uint8_t* physaddr;
    uint8_t* virtaddr;
    size_t len;
    bool initialized;

    bool in32(long offset, uint32_t* res) const;
    bool out32(long offset, uint32_t val) const;
};

GPIO_Device::GPIO_Device(const char* device_name) : physaddr(nullptr), virtaddr(nullptr), len(0), initialized(false) {
    snprintf(name, GPIO_NAME_MAX_LEN, "%s", device_name);
    
}

GPIO_Device::~GPIO_Device() {
    if (initialized) {
        l4io_release_iomem(reinterpret_cast<l4_addr_t>(virtaddr), len);
    }
}

bool GPIO_Device::init() {
    if (initialized) {
        std::cout << __FUNCTION__ << " GPIO device already initialized: " << name << std::endl;
        return true;
    }

    if (!res.init(name)) {
        std::cout << "Failed to get GPIO resources for device: " << name << std::endl;
        return false;
    }

    len = res.mem_end - res.mem_start + 1;
    l4_addr_t virtaddr_temp = 0;
    if (l4io_request_iomem(res.mem_start, len, L4IO_MEM_NONCACHED, &virtaddr_temp) != 0) {
        return false;
    }

    physaddr = reinterpret_cast<uint8_t*>(res.mem_start);
    virtaddr = reinterpret_cast<uint8_t*>(virtaddr_temp);
    initialized = true;
    return true;
}

bool GPIO_Device::check(const char* fn) const {
    if (!initialized) {
        std::cout << fn << ": Object not initialized" << std::endl;
        return false;
    }
    return true;
}

bool GPIO_Device::setDirection(uint32_t channel, uint32_t directionmask) {
    if (!check(__FUNCTION__)) return false;
    const uint8_t XGPIO_CHAN_OFFSET = 8;
    const uint32_t XGPIO_TRI_OFFSET = 0x4;
    return out32(((channel - 1) * XGPIO_CHAN_OFFSET) + XGPIO_TRI_OFFSET, directionmask);
}

bool GPIO_Device::discreteWrite(unsigned channel, uint32_t data) {
    if (!check(__FUNCTION__)) return false;
    if (channel == 0) {
        std::cout << "channel num must start with 1" << std::endl;
        return false;
    }
    return out32((channel - 1) * 8 + 0x0, data);
}

bool GPIO_Device::discreteRead(unsigned channel, uint32_t* data) {
    if (!check(__FUNCTION__)) return false;
    if (channel == 0) {
        std::cout << "channel num must start with 1" << std::endl;
        return false;
    }
    return in32((channel - 1) * 8 + 0x0, data);
}

bool GPIO_Device::interruptEnable(uint32_t mask) {
    if (!check(__FUNCTION__)) return false;
    const uint32_t GPIO_IER_OFFSET = 0x128;
    uint32_t reg = 0;
    if (!in32(GPIO_IER_OFFSET, &reg)) {
        std::cout << "Cannot get interrupt register" << std::endl;
        return false;
    }
    return out32(GPIO_IER_OFFSET, reg | mask);
}

bool GPIO_Device::interruptDisable(uint32_t mask) {
    if (!check(__FUNCTION__)) return false;
    const uint32_t GPIO_IER_OFFSET = 0x128;
    uint32_t reg = 0;
    if (!in32(GPIO_IER_OFFSET, &reg)) {
        std::cout << "Cannot get interrupt register" << std::endl;
        return false;
    }
    return out32(GPIO_IER_OFFSET, reg & (~mask));
}

bool GPIO_Device::interruptClear(uint32_t mask) {
    if (!check(__FUNCTION__)) return false;
    const uint32_t GPIO_ISR_OFFSET = 0x120;
    uint32_t reg = 0;
    if (!in32(GPIO_ISR_OFFSET, &reg)) {
        std::cout << "Cannot get interrupt register" << std::endl;
        return false;
    }
    return out32(GPIO_ISR_OFFSET, reg & mask);
}

bool GPIO_Device::globalInterruptEnable() {
    if (!check(__FUNCTION__)) return false;
    const uint32_t GPIO_GIE_OFFSET = 0x11C;
    const uint32_t GPIO_GIE_GINTR_ENABLE_MASK = 0x80000000;
    return out32(GPIO_GIE_OFFSET, GPIO_GIE_GINTR_ENABLE_MASK);
}

void GPIO_Device::showInfo() const {
    if (!check(__FUNCTION__)) return;
    std::cout << "GPIO Device Info:" << std::endl;
    std::cout << " Name: " << name << std::endl;
    std::cout << " Memory Resource: 0x" << std::hex << res.mem_start << " - 0x" << res.mem_end << std::endl;
    std::cout << " IRQ Resource: 0x" << std::hex << res.irq_start << " - 0x" << res.irq_end << std::endl;
    std::cout << " physaddr: 0x" << std::hex << reinterpret_cast<unsigned long>(physaddr) << ", VirtualAddress: 0x" << reinterpret_cast<unsigned long>(virtaddr) << ", length: 0x" << len << std::endl;
    std::cout << " Initialized: " << (initialized ? "Yes" : "No") << std::endl;
}

bool GPIO_Device::in32(long offset, uint32_t* res) const {
    if (!res) {
        PRINT_INVALED_PARAMETERS;
        return false;
    }
    volatile uint32_t* mem = reinterpret_cast<volatile uint32_t*>(virtaddr + offset);
    uint32_t val = 0;
#ifdef __aarch64__
    asm volatile("ldr %w[val], %[mem]" : [val] "=r"(val) : [mem] "m"(*mem));
#else
#error Architecture not supported
#endif
    *res = val;
    return true;
}

bool GPIO_Device::out32(long offset, uint32_t val) const {
    volatile uint32_t* mem = reinterpret_cast<volatile uint32_t*>(virtaddr + offset);
#ifdef __aarch64__
    asm volatile("str %w[val], %[mem]" : [mem] "=m"(*mem) : [val] "r"(val));
#else
#error Architecture not supported
#endif
    return true;
}

//////////////// C interface ////////////////
#ifdef __cplusplus
extern "C" {
#endif


bool GPIO_Device_create(const char* device_name, void** dev) {
    if (!device_name || !dev) {
        PRINT_INVALED_PARAMETERS;
        return false;
    }
    try {
        *dev = new GPIO_Device(device_name);
        GPIO_Device* device = static_cast<GPIO_Device*>(*dev);
        if (!device->init()) {
            delete device;
            *dev = nullptr;
            return false;
        }
        return true;
    } catch (const std::bad_alloc&) {
        std::cout << "Failed to allocate GPIO_Device for: " << device_name << std::endl;
        return false;
    }
}

bool GPIO_Device_check(void* device_) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->check(__FUNCTION__);
}

bool GPIO_Device_free(void** device) {
    if (!device || !*device) {
        PRINT_INVALED_PARAMETERS;
        return false;
    }
    delete static_cast<GPIO_Device*>(*device);
    *device = nullptr;
    return true;
}

bool GPIO_Device_show_info(void* device_) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    device->showInfo();
    return true;
}

bool GPIO_Device_setdirection(void* device_, uint32_t channel, uint32_t directionmask) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->setDirection(channel, directionmask);
}

bool GPIO_Device_DiscreteWrite(void* device_, unsigned channel, uint32_t data) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->discreteWrite(channel, data);
}

bool GPIO_Device_DiscreteRead(void* device_, unsigned channel, uint32_t* data) {
    if (!device_ || !data) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->discreteRead(channel, data);
}

bool GPIO_Device_InterruptEnable(void* device_, uint32_t mask) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->interruptEnable(mask);
}

bool GPIO_Device_InterruptDisable(void* device_, uint32_t mask) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->interruptDisable(mask);
}

bool GPIO_Device_InterruptClear(void* device_, uint32_t mask) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->interruptClear(mask);
}

bool GPIO_Device_GlobalInterruptEnable(void* device_) {
    if (!device_) return false;
    GPIO_Device* device = static_cast<GPIO_Device*>(device_);
    return device->globalInterruptEnable();
}

#ifdef __cplusplus
}
#endif