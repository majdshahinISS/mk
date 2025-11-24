#include "../include/MS_Types.h"
#ifdef __cplusplus
extern "C" {
#endif


struct type_only{const unsigned int type_num;};

bool check_type(void *obj, unsigned int type)
{
    if(obj == nullptr)
        return false;
    struct type_only * to = (struct type_only*)obj;
    unsigned int obj_type = to->type_num;
    if(obj_type == type)
        return true;
    char * name;
    switch (type)
    {
    case A_type:
        name = "A_type";
        break;
    
    default:
        name = "Unknown type";
        break;
    }
    l4re_log_printf("False usage of type: %s\n\r",name);
    return false;
}

#ifdef __cplusplus
}
#endif