#include "../include/log_wrapper.h"
#include <stdarg.h>
#include <stdio.h>
// C interface for L4Re::Log
extern "C" {

void l4re_log_print(const char *msg)
{
  if (!msg)
    return;
  auto log = L4Re::Env::env()->log();
  log->print(msg);
}

unsigned long l4re_get_log_cap(void)
{
  return L4Re::Env::env()->log().cap();
}



void l4re_log_printf(const char *format, ...)
{
    char buffer[256];  // Adjust size as needed
    va_list args;
    
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        // Ensure null termination and prevent buffer overflow
        if (len >= (int)sizeof(buffer))
            buffer[sizeof(buffer) - 1] = '\0';
    }
    else{
      buffer[len]='\0';
    }
    l4re_log_print(buffer);
}
}
