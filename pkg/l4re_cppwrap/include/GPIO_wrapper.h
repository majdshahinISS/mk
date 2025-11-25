#ifndef __MS_GPIO_H
#define __MS_GPIO_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif
#define GPIO_NAME_MAX_LEN   16
#define GPIO_All_Output     0x00000000
#define GPIO_All_Input      0xFFFFFFFF

bool GPIO_Device_create( const char* device_name, void ** dev);
bool GPIO_Device_check(void * device_);
// bool init_gpio_resources(const char* device_name, GPIO_Resource *res_out);
// bool GPIO_Device_init(void* device);
bool GPIO_Device_free(void** device);// REVIEW 
bool GPIO_Device_show_info(void* device);

// channel start of index 1
bool GPIO_Device_setdirection(void * device_, uint32_t channel, uint32_t directionmask);
//bool _Out32(void * device_ , long offset, uint32_t val);
bool GPIO_Device_DiscreteWrite(void * device, unsigned channel, uint32_t data);
bool GPIO_Device_DiscreteRead(void * device, unsigned channel, uint32_t * data);

bool GPIO_Device_InterruptEnable(void * device, uint32_t mask);
bool GPIO_Device_InterruptDisable(void * device, uint32_t mask);
bool GPIO_Device_InterruptClear(void * device, uint32_t mask);
bool GPIO_Device_GlobalInterruptEnable(void * device);
#ifdef __cplusplus
}
#endif
#endif // __MS_GPIO_H