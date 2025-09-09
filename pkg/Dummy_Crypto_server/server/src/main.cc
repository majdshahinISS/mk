#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>


#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>
#include <l4/re/env>
#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/util/cap_alloc>

#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>
#include <l4/sys/cxx/ipc_epiface>

#include "/home/iss/L4Re_d/l4/pkg/user_shared/crypto_shared.h"

#include <l4/re/util/cap_alloc> // For capability allocation
#include <l4/re/dataspace>      // For Dataspace interface
#include <l4/re/mem_alloc>      // For the memory allocator

static L4Re::Util::Registry_server<> server;
 
class Calculation_server : public L4::Epiface_t<Calculation_server, Calc>
{
public:
  int op_sub(Calc::Rights, l4_uint32_t a, l4_uint32_t b, l4_uint32_t &res)
  {
    res = a - b;
    return 0;
  }
 
  int op_neg(Calc::Rights, l4_uint32_t a, l4_uint32_t &res)
  {
    res = -a;
    return 0;
  }
};

int
main()
{

  constexpr l4_size_t Size = 4096; // one page, adjust as needed

  // allocate a cap slot for the dataspace
  L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds.is_valid()) {
    std::printf("cap alloc failed\n");
    return 1;
  }

  // create the dataspace from the root memory allocator
  long err = L4Re::Env::env()->mem_alloc()->alloc(Size, ds, 0);
  if (err < 0) {
    std::printf("mem_alloc->alloc failed: %ld\n", err);
    return 1;
  }

  std::printf("dataspace created, size=%lu bytes\n",
              static_cast<unsigned long>(ds->size()));
  return 0;

/*

  static Calculation_server calc;
 
  // Register calculation server
  if (!server.registry()->register_obj(&calc, "crypto_ipc").is_valid())
    {
      printf("Could not register my service, is there a 'crypto_ipc' in the caps table?\n");
      return 1;
    }
 
  printf("Welcome to the calculation server!\n"
         "I can do subtractions and negations.\n");
 
  // Wait for client requests
  server.loop();
 */
  return 0;
}