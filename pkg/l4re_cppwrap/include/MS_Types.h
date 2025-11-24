#ifndef __MS__TYPES__
#define __MS__TYPES__

#include"log_wrapper.h"

#include <cstdlib>
#ifdef __cplusplus
extern "C" {
#endif

#define A_type 0x1258fffa
#define OBJ_TYPE_GPIO_Device 0x47504F20 // 'GPO '


bool check_type(void *obj, unsigned int type);
#ifdef __cplusplus
}
#endif
#endif