#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>


#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
 
#include <stdio.h>

#include <l4/util/util.h>
#include "/home/iss/L4Re_d/l4/pkg/user_shared/crypto_shared.h"

int
main()
{
  sleep(1);
  L4::Cap<ICrypto> crypto =L4Re::Env::env()->get_cap<ICrypto>("crypto_ipc");
  if (!crypto.is_valid()) {
    std::printf("Failed to get crypto capability\n");
    return 1;
  }
  int x = 0;
  crypto->dummy(x );
  std::printf("dummy returned x = %d\n", x); // ok it prints 3

  L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  L4::Ipc::Cap<L4Re::Dataspace> ds_ipc(ds);
  int r = crypto->getDS(ds_ipc); // ds_ipc
  if (r != L4_EOK) {
    std::printf("getDS failed: error : 0x%x\n", r);
    return 1;
  }
  else 
  {
    std::printf("getDS succeeded, dataspace size=%lu bytes\n",
                static_cast<unsigned long>(ds->size())); // ok but it prints 0
  } 

  /*
  L4::Cap<L4Re::Dataspace> ds;
  int res = crypto->getDS(ds);
  if (res < 0) {
    std::printf("getDS failed\n");
    return 1;
  }
  else 
  {
    std::printf("getDS succeeded, dataspace size=%lu bytes\n",
                static_cast<unsigned long>(ds->size())); // ok but it prints 0
  }

  void *buf = nullptr; l4_size_t sz = ds->size();
  if (L4Re::Env::env()->rm()->attach(&buf, sz,
        L4Re::Rm::F::Search_addr|L4Re::Rm::F::RW,
        L4::Ipc::make_cap_rw(ds)) < 0) {
    std::printf("Failed to attach dataspace\n");// Error 
    return 1;
  }
  std::printf("Dataspace attached at %p, size=%lu bytes\n", buf,
              static_cast<unsigned long>(sz));

  std::printf("getDS succeeded, dataspace size=%lu bytes\n",
              static_cast<unsigned long>(ds->size()));
  if (!ds.is_valid()) {
    std::printf("Invalid dataspace capability received\n");
    return 1;
  }
*/
 
  return 0;
}