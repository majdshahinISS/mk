#include "../include/MS_Types.h"
#include "../include/dummy.h"
#include "../include/log_wrapper.h"
// C interface for L4Re::Log
#ifdef __cplusplus
extern "C" {
#endif


struct A{
    unsigned int type=A_type;
    int data[32];
};

bool dummy_init(void *obj)
{
    if(check_type(obj, A_type)==false)
        return false;
    struct A * a = (struct A*)obj;
    for (int i = 0; i < 32 ; i++) a->data[i] = i;
    return true;
}

bool dummy_print(void *obj)
{
    l4re_log_printf("printing...\n");
    if(check_type(obj,A_type)==false)
        return false;
    struct A * a = (struct A*)obj;
    for (int i = 0; i < 32 ; i++)
        l4re_log_printf("%d,",a->data[i]);
    l4re_log_printf("\n\r");
    return true;
}

bool dummy_new_obj(void **obj)
{
    void *mem = malloc(sizeof(A));
    if(mem == nullptr)
    {
        l4re_log_printf("no enough memory\n");
        return false;
    }
    
    A * p = (A*)mem;
    p->type = A_type;
    (*obj)=mem;
    return true;
}
bool dummy_free(void **obj)
{
    if (obj == NULL || *obj == NULL) {
        // nothing to free
        return false;
    }

    if (check_type(*obj, A_type) == false)
        return false;

    free(*obj);   // free real pointer
    *obj = NULL;  // ✔ write null back to caller

    return true;
}


#ifdef __cplusplus
}
#endif