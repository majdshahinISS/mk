#ifndef __MS__DUMMY__
#define __MS__DUMMY__
#include <l4/re/env>
#include <l4/re/log>
#include <l4/sys/cxx/ipc_epiface>
#ifdef __cplusplus
extern "C" {
#endif


bool dummy_init(void *obj);
bool dummy_print(void *obj);
bool dummy_new_obj(void **obj);
bool dummy_free(void **obj);

#ifdef __cplusplus
}
#endif
#endif