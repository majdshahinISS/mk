#ifndef L4RE_CPPWRAP_LOG_WRAPPER_H
#define L4RE_CPPWRAP_LOG_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void l4re_log_print(const char *msg);
unsigned long l4re_get_log_cap(void);

#ifdef __cplusplus
}
#endif

#endif
