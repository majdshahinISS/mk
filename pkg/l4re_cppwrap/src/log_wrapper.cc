#include "../include/log_wrapper.h"

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

}
