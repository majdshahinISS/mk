#ifndef L4RE_CPPWRAP_LOG_WRAPPER_H
#define L4RE_CPPWRAP_LOG_WRAPPER_H
#include <l4/re/env>
#include <l4/re/log>
#include <l4/sys/cxx/ipc_epiface>
#ifdef __cplusplus
extern "C" {
#endif

void l4re_log_print(const char *msg);
unsigned long l4re_get_log_cap(void);

#ifdef __cplusplus
}
#endif

#endif
