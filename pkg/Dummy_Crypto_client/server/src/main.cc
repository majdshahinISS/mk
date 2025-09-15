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
  L4::Cap<IDataspaceOwner> dss =L4Re::Env::env()->get_cap<IDataspaceOwner>("crypto_ipc");
  if (!dss.is_valid()) {
    std::printf("Failed to get dss capability\n");
    return 1;
  }
  dss->init(4096, 1000); // create a dataspace of 4096 bytes
  L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  int r = dss->getDS(ds); 
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
      L4::Ipc::make_cap(ds, L4_CAP_FPAGE_RW));                    // grant RW rights
  if (err < 0) {
    std::printf("attach_ds: attach failed (%ld)\n", err);
    return 1;
  }
  std::printf("attach_ds: attached at %p, size=%lu\n",
              addr, static_cast<unsigned long>(size));
  // write to the dataspace
  int i = 0;
  while (true)
  {
    // write to the dataspace
    if (addr) {
      std::printf("Client: writing to dataspace at %p, size=%lu\n",
                  addr, static_cast<unsigned long>(size));
      char buffer[100];
      std::snprintf(buffer, sizeof(buffer), "Hello from Client: %d", i++ );
      std::strcpy(static_cast<char*>(addr),  buffer);
      dss->new_data(i,1,0, std::strlen(buffer)+1); // notify server that new data is available
      dss->free_your_data(i,1,0, std::strlen(buffer)+1); // notify server to free the data
    }
    else {
      std::printf("Client: dataspace pointer is null\n");
      return 1;
    }
    sleep(1);
    if (i == 5) break;
  }
  

 
  /*
  L4::Cap<ICrypto> crypto =L4Re::Env::env()->get_cap<ICrypto>("crypto_ipc");
  if (!crypto.is_valid()) {
    std::printf("Failed to get crypto capability\n");
    return 1;
  }
  int x = 0;
  crypto->dummy(x );
  std::printf("dummy returned x = %d\n", x); // ok it prints 3

  L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  int r = crypto->CTS_getDS(ds); 
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
      L4::Ipc::make_cap(ds, L4_CAP_FPAGE_RW));                    // grant RW rights
  if (err < 0) {
    std::printf("attach_ds: attach failed (%ld)\n", err);
    return 1;
  }
  int i = 0;
  l4_size_t write_index = 0;
  while (addr) {
    std::printf("Client: writing to dataspace at %p, size=%lu\n",
                addr, static_cast<unsigned long>(size));
    char buffer[100];
    std::snprintf(buffer, sizeof(buffer), "Hello from Client %d", i++);
    l4_size_t str_len = std::strlen(buffer);

    if( write_index + str_len +1 > size) {
      std::printf("Client: no more space in dataspace\n");
      break;
    }

    std::strcpy(static_cast<char*>(addr + write_index),  buffer);

    crypto->CTS_ready(i,1,write_index, str_len); // notify server that client to server is ready
    write_index += ((str_len+1 +3)/4)*4; // align to 4 bytes

    sleep(1);
    if (i == 5) break;
  }*/
  return 0;
}