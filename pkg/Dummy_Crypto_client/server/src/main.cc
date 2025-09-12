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
  int r = crypto->getDS(ds); 
  if (r != L4_EOK) {
    std::printf("getDS failed: error : 0x%x\n", r);
    return 1;
  }
  else 
  {
    std::printf("getDS succeeded, dataspace size=%lu bytes\n",
                static_cast<unsigned long>(ds->size())); // ok but it prints 0
  } 
  // attach the dataspace
  void *addr = nullptr;
  l4_size_t size = ds->size();
  long err = L4Re::Env::env()->rm()->attach(
      &addr, size,
      L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
      L4::Ipc::make_cap_rw(ds));                    // grant RW rights
  if (err < 0) {
    std::printf("attach_ds: attach failed (%ld)\n", err);
    return 1;
  }
  int i = 0;
  while (addr) {
    std::printf("Client: writing to dataspace at %p, size=%lu\n",
                addr, static_cast<unsigned long>(size));
    char buffer[100];
    std::snprintf(buffer, sizeof(buffer), "Hello from Client %d", i++);
    std::strcpy(static_cast<char*>(addr), buffer);
    crypto->CTS_ready(size); // notify server that client to server is ready
    sleep(1);
    if (i == 5) break;
  }
  return 0;
}